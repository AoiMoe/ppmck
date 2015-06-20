;-----------------------------------------------------------------------
;FDS(2C33) driver
;-----------------------------------------------------------------------

;--------------------
; fds_sound_init : FDS音源の初期化
;
; 副作用:
;	a : 破壊
;	音源 : 初期化
;
fds_sound_init:
	lda #%00000010		; 2C33 Sound Enable
	sta $4023

	lda	#$00
	sta	$4080
	sta	$4082
	sta	$4083
	sta	$4084
	sta	$4085
	sta	$4086
	sta	$4087
	sta	$4088
	sta	$4089
	lda	#$e8
	sta	$408a
	rts


;--------------------
; sound_fds : NMI割り込みエントリポイント
;
; 備考:
;	XXX:サブルーチン名
;
sound_fds:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	fds_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go:
	jsr	sound_fds_read
	jsr	fds_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	jsr	sound_fds_write		;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done
	rts


;--------------------
; fds_do_effect : 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
fds_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	;FDSにデューティー比は無い

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_fds_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_fds_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_fds_pitch_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.hardenve_write
	; 同一フレームのsound_fds_readの処理でキーオンが行われたかどうかで
	; ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	; キーオンが行われてないフレームは通常の処理
	jsr	sound_fds_note_enve
	jmp	.hardenve_write
.arpe_key_on:
	; キーオンが行われたフレームはワークエリアの調整のみ行う
	; 実際にレジスタに反映するのは sound_fds の最後
	jsr	note_enve_sub
	jsr	fds_freq_set
	jsr	arpeggio_address

.hardenve_write:
	lda	effect_flag,x
	and	#EFF_DUTYENV_ENABLE
	beq	.done
	jsr	sound_fds_hard_enve
	ldx	<channel_selx2

.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; fds_freq_set : ノート番号を周波数データに変換する
;
; 入力:
;	sound_sel,x : 現在のノート番号
;	detune_dat,x : 符号付きデチューン値(detune_write_subへの間接的入力)
; 出力:
;	sound_freq_{low,high},x : 周波数データ
; 副作用:
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
; 備考:
;	このサブルーチンは音源レジスタへの書き込みは行わない
;
fds_freq_set:
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	;音階→周波数変換テーブルのオフセット計算
	lda	.frequency_table,y	;FDS周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	.frequency_table+1,y	;FDS周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

	;オクターブ処理 - 内蔵音源とは方向が逆
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	cmp	#$06
	beq	.done			;6ならそのまま終わり

	tay
.oct_loop:
	;o6を基準に、1オクターブ下がるごとに分周器の設定値を1/2する
	lsr	sound_freq_high,x	;符号なし16bit右シフト
	ror	sound_freq_low,x	;
	iny				;
	cpy	#$06
	bne	.oct_loop		;オクターブ分繰り返す

.done:
	jsr	detune_write_sub
	rts

.frequency_table:
	dw	$0995,$0a26,$0ac0,$0b64	;c ,c+,d ,d+
	dw	$0c11,$0cc9,$0d8c,$0e5a	;e ,f ,f+,g
	dw	$0f35,$101c,$1110,$1214	;g+,a ,a+,b
	dw	$0000,$0000,$0000,$0000

;平均律（に近い数値）を基準にしています
;式は以下な感じで
;              111860.8 Hz
;再生周波数 = ------------- x n(周波数用データ)
;		64 x 4096
;o6aより上の音はでません（テーブルはo6のモノ）
;音程が下がるほど音痴になります？
;
;        ユニゾン：	x1.0000
;        短２度：	x1.0595
;        長２度：	x1.1225
;        短３度：	x1.1892
;        長３度：	x1.2599
;        完全４度：	x1.3348
;        増４度(減５度):x1.4142
;        完全５度：	x1.4983
;        増５度(短６度):x1.5874
;        長６度：	x1.6818
;        減７度：	x1.7818
;        長７度：	x1.8877
;        オクターブ ：	x2.0000


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_fds_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_fds_read:
.next_cmd:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]

;----------
;ループ処理1
.loop_program:
	cmp	#CMD_LOOP1
	bne	.loop_program2
	jsr	loop_sub
	jmp	.next_cmd

;----------
;ループ処理2(分岐)
.loop_program2:
	cmp	#CMD_LOOP2
	bne	.bank_set
	jsr	loop_sub2
	jmp	.next_cmd

;----------
;バンク切り替え
.bank_set:
	cmp	#CMD_BANK_SWITCH
	bne	.wave_set
	jsr	data_bank_addr
	jmp	.next_cmd

;----------
;データエンド設定
;.data_end:
;	cmp	#CMD_END
;	bne	.wave_set
;	jsr	data_end_sub
;	jmp	.next_cmd

;----------
;音色設定
;XXX:サブルーチン化したほうがいい
.wave_set:
	cmp	#CMD_TONE
	bne	.volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]

	;16bit配列のオフセットに変換してインデックスレジスタXに格納する
	asl	a
	tax

	;バンクをFDS音色テーブルに切り替える
	lda	#bank(fds_data_table)*2
	jsr	change_bank

	;波形データソース開始アドレスをtemp_data_add(16bit)に格納する
	lda	fds_data_table,x
	sta	<temp_data_add
	inx
	lda	fds_data_table,x
	sta	<temp_data_add+1

	;波形データ転送
	ldy	#$00			;オフセット
	ldx	#$00			;XXX:無駄
	lda	#%10000000		;波形テーブルRAMへの書き込み許可
	sta	$4089			;
	;64バイト分繰り返す
.wave_loop:
	lda	[temp_data_add],y
	sta	$4040,y
	iny
	cpy	#$40
	bne	.wave_loop

	lda	#%00000000		;波形テーブルRAMへの書き込み禁止
	sta	$4089			;

	ldx	<channel_selx2
	jsr	sound_data_address
	jmp	.next_cmd

;----------
;音量設定
.volume_set:
	cmp	#CMD_VOLUME
	bne	.rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	bpl	.softenve_part		;ソフトエンベ処理へ

	;音量直接指定
.volume_part:
	;XXX:おそらく以前はコメントアウトされたコードが生きていたが、
	;    ハードウェアレジスタへの反映を後回しにするよう変更された模様。
	;    綺麗に書き直すべし。
;	ora	#%10000000		;常に直接モード
	and	#%10111111
	sta	fds_volume
;	sta	$4080			;ボリューム＆ハードエンベ書き込み

	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	jsr	sound_data_address
	jmp	.next_cmd

	;ソフトエンベ有効化
	;XXX:全く同じことをしているのでvolume_subを使うべし
.softenve_part:
	sta	softenve_sel,x		;128-255が格納される
	asl	a			;どうせMSBはここで失われる
	tay				;y = ソフトエンベロープ番号*2

	;バンクをソフトエンベテーブルに切り替える
	lda	#bank(softenve_table)*2
	jsr	change_bank

	lda	softenve_table,y	;ソフトエンベデータアドレス設定
	sta	soft_add_low,x
	lda	softenve_table+1,y
	sta	soft_add_low+1,x

	lda	effect_flag,x
	ora	#EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ有効指定

	jsr	sound_data_address
	jmp	.next_cmd

;----------
;休符
.rest_set:
	cmp	#CMD_REST
	bne	.lfo_set

	;休符フラグを立てる
	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	;ウェイトを設定
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	;音を停止する
	lda	#$00
	sta	$4080

	jsr	sound_data_address
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ピッチLFO設定
.lfo_set:
	cmp	#CMD_SOFTLFO
	bne	.detune_set
	jsr	lfo_set_sub
	jmp	.next_cmd

;----------
;デチューン設定
.detune_set:
	cmp	#CMD_DETUNE
	bne	.pitch_set
	jsr	detune_sub
	jmp	.next_cmd

;----------
;ピッチエンベロープ設定
.pitch_set:
	cmp	#CMD_PITCHENV
	bne	.arpeggio_set
	jsr	pitch_set_sub
	jmp	.next_cmd

;----------
;ノートエンベロープ設定
.arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	.freq_direct_set
	jsr	arpeggio_set_sub
	jmp	.next_cmd

;----------
;再生周波数直接設定
.freq_direct_set:
	cmp	#CMD_DIRECT_FREQ
	bne	.y_command_set
	jsr	direct_freq_sub
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ｙコマンド設定
.y_command_set:
	cmp	#CMD_WRITE_REG
	bne	.wait_set
	jsr	y_sub
	jmp	.next_cmd

;----------
;ウェイト設定
.wait_set:
	cmp	#CMD_WAIT
	bne	.hard_lfo_set
	jsr	wait_sub
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;FDSハードウェアエフェクト設定
.hard_lfo_set:
	cmp	#CMD_FDS_HWLFO
	bne	.hwenv_set

	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	.hard_lfo_data_set

	;ハードウェアLFOの無効化
	ldx	<channel_selx2
	lda	effect_flag,x
	and	#~EFF_DUTYENV_ENABLE
	sta	effect_flag,x

	jsr	sound_data_address
	jmp	.next_cmd

	;ハードウェアLFOデータの設定
.hard_lfo_data_set:
	asl	a
	asl	a
	asl	a
	asl	a
	tay				;y = LFOデータ番号*16

	;バンクをハードウェアLFOデータに切り替える
	lda	#bank(fds_effect_select)*2
	jsr	change_bank

	;XXX:壊れている。fds_hard_selectにはLFOデータ番号*16+1を格納しないといけないが、バンク切り替えのコードでaレジスタは破壊されている。つまり、staをstyにすべし。
	sta	fds_hard_select
	inc	fds_hard_select

	;ディレイカウント
	lda	fds_effect_select,y
	sta	fds_hard_count_1
	sta	fds_hard_count_2

	ldx	<channel_selx2
	lda	effect_flag,x
	ora	#EFF_DUTYENV_ENABLE
	sta	effect_flag,x

	jsr	sound_data_address
	jmp	.next_cmd

;----------
;ハードウェアボリュームエンベロープ
.hwenv_set:
	cmp	#CMD_HWENV
	bne	.modfreq_set

	jsr	sound_data_address
	lda	[sound_add_low,x]

	;XXX:おそらく以前はコメントアウトされたコードが生きていたが、
	;    ハードウェアレジスタへの反映を後回しにするよう変更された模様。
	;    綺麗に書き直すべし。
	and	#%01111111		;一応
	sta	fds_volume
;	sta	$4080			;ボリューム＆ハードエンベ書き込み

	;ソフトエンベ無効指定
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x

	jsr	sound_data_address
	jmp	.next_cmd

;----------
;変調周波数設定
;XXX:サブルーチン化したほうがいい
.modfreq_set:
	cmp	#CMD_FDS_MODFREQ
	bne	.keyon_set

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	$4086			; MOD freq low

	jsr	sound_data_address
	lda	[sound_add_low,x]
	and	#%00001111
	sta	$4087			; MOD freq high

	jsr	sound_data_address
	jmp	.next_cmd

;----------
.keyon_set:
	;XXX:知らないコマンドが来たときの処理はあったほうが良いかも
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	lda	#$00
	sta	$4083
;	sta	rest_flag,x		;effect_initでやってるのでいらない
	jsr	fds_freq_set		;周波数セットへ

	;音量レジスタの設定
	;(.volume_setや.hwenv_setから遅延された処理)
	lda	fds_volume
	sta	$4080

	jsr	effect_init
	ldx	<channel_selx2

	;ハードウェアLFOの設定
	lda	fds_hard_count_2	;ディレイカウントの初期化
	sta	fds_hard_count_1	;
	lda	#$00
	sta	$4084			;LFO無効化 - ディレイ後に有効化
	sta	$4085			;
	sta	$4086			;
	sta	$4087			;

	rts				;音長を伴うコマンドなのでこのまま終了


;-------------------------------------------------------------------------------
sound_fds_write:
	ldx	<channel_selx2
	lda	sound_freq_low,x	;Low Write
	sta	$4082
	lda	sound_freq_high,x	;High Write
	sta	$4083
	rts
;-------------------------------------------------------------------------------
sound_fds_softenve:
	jsr	volume_enve_sub
	and	#%00111111
	ora	#$80
	sta	$4080
	jsr	enverope_address
	rts
;-------------------------------------------------------------------------------
sound_fds_lfo:
	jsr	lfo_sub
	jsr	sound_fds_write
	rts
;-------------------------------------------------------------------------------
sound_fds_pitch_enve:
	jsr	pitch_sub
	jsr	sound_fds_write
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
sound_fds_note_enve
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	fds_freq_set
	jsr	sound_fds_write
	jsr	arpeggio_address
	rts
.end4
;	jsr	fds_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
sound_fds_hard_enve:
	lda	fds_hard_count_1
	beq	start_effect
	cmp	#$ff
	beq	return6809
	dec	fds_hard_count_1
return6809:
	rts
start_effect:
	lda	fds_hard_select
	tax
start_effect_2:
	lda	fds_effect_select,x
	tay
	inx
	cmp	#$ff
	beq	count_effect
	cmp	#$88
	beq	set_4088
	lda	fds_effect_select,x
	sta	$4000,y
	inx
	jmp	start_effect_2

count_effect:
	dec	fds_hard_count_1
	rts
set_4088:
	lda	fds_effect_select,x
	stx	temporary

	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	tay
	ldx	#$00
fds_4088_write:
	; 定義バンク切り替え
	lda	#bank(fds_4088_data)*2
	jsr	change_bank

	lda	fds_4088_data,y
	sta	$4088
	iny
	inx
	cpx	#$20
	beq	oshimai
	jmp	fds_4088_write
oshimai:
	ldx	temporary
	inx
	jmp	start_effect_2
;-------------------------------------------------------------------------------
