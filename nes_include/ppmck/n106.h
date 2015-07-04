;-----------------------------------------------------------------------
;NAMCOT163(a.k.a.N106) driver
;-----------------------------------------------------------------------

;--------------------
; n106_sound_init : N163の初期化
;
; 副作用:
;	a : 破壊
;	N163 : 初期化
;
n106_sound_init:
	;$E000の設定
	;Sunsoft5Bを使用する場合はアドレスが重複しているので書かない
	;NSFプレイヤーのほうで良きに計らってくれるのだろう
	;XXX:.if でよさそう
	lda	# (SOUND_GENERATOR & __FME7)
	bne	.skip_enable

	;マッパーのセット/音源無効化フラグのクリア
	lda	#%00100000
		; .SPPPPPP
		;  |||||||
		;  |++++++-- select 8kb page of PRG-ROM at $8000
		;  +-------- disable sound if set
		;  (from nesdev.com)
	sta	$E000
.skip_enable:

	;N163内部メモリの初期化
	;アドレス$00-$7Eまでをゼロクリアしている
	ldx	#$7f		;XXX:ループ1回足りないと思う
	lda	#%10000000	;$00から自動インクリメントで書き込む
		; IAAAAAAA
		; ||||||||
		; |+++++++-- address of n163 internal memory
		; +--------- auto increment
		;  (from nesdev.com)
	sta	$f800		;address port
	lda	#$00
.init_loop:
	sta	$4800		;data port
	dex
	bne	.init_loop

	;チャンネル数の設定
	;有効なチャンネル数は、内部メモリアドレス$7Fのbit4-6に設定する
	;(0:1ch, 1:2ch, ... 7:8ch)
	lda	#$7f
	sta	$f800

	;バンクを切り替えてn106_channelを読み込む
	lda	#bank(n106_channel)*2
	jsr	change_bank
	lda	n106_channel

	;n106_channelの値を1引いて4ビット左にシフトする
	sec
	sbc	#$01
	asl	a
	asl	a
	asl	a
	asl	a
	sta	$4800

	;$7Fのbit0-4はch1の音量レジスタでもあるので、orするために保存する
	sta	n106_7f

	rts


;--------------------
; sound_n106 : NMI割り込みエントリポイント
;
; 備考:
;	XXX:サブルーチン名
;
sound_n106:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	n106_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go:
	jsr	sound_n106_read
	jsr	n106_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	jsr	sound_n106_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done
	rts


;--------------------
; n106_do_effect : 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
n106_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	;N163にデューティー比は無い

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_n106_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_n106_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_n106_pitch_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.done
	;同一フレームのsound_n106_readの処理でキーオンが行われたかどうかで
	;ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	;キーオンが行われてないフレームは通常の処理
	jsr	sound_n106_note_enve
	jmp	.done
.arpe_key_on:
	;キーオンが行われたフレームはワークエリアの調整のみ行う
	;実際にレジスタに反映するのは sound_n106 の最後
	jsr	note_enve_sub
	jsr	n106_freq_set
	jsr	arpeggio_address

.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; n106_freq_set : ノート番号を周波数データに変換する
;
; 入力:
;	sound_sel,x : 現在のノート番号
;	detune_dat,x : 符号付きデチューン値(detune_write_subへの間接的入力)
; 出力:
;	sound_freq_{low,high,n106},x : 周波数データ
; 副作用:
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
; 備考:
;	このサブルーチンは音源レジスタへの書き込みは行わない
;
n106_freq_set:
	ldx	<channel_selx2

	;音階→周波数変換テーブルのオフセット計算
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	asl	a
	tay

	lda	.frequency_table,y	;n106周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	.frequency_table+1,y	;n106周波数テーブルからMidleを読み出す
	sta	sound_freq_high,x	;書き込み
	lda	.frequency_table+2,y	;n106周波数テーブルからHighを読み出す
	sta	sound_freq_n106,x	;書き込み

	;オクターブ処理 - 内蔵音源とは方向が逆
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	sta	<drvtmp0
	cmp	#$08
	beq	.done			;8ならそのまま終わり

	tay				;y=オクターブ値(ループ変数)
.oct_loop:
	;o8を基準に、1オクターブ下がるごとに分周器の設定値を1/2する
	lsr	sound_freq_n106,x	;符号なし24bit右シフト
	ror	sound_freq_high,x	;
	ror	sound_freq_low,x	;
	iny				;
	cpy	#$08
	bne	.oct_loop		;オクターブ分繰り返す

.done:
	.if PITCH_CORRECTION
		;detune_write_sub側で補正する
		jsr	detune_write_sub
	.else
		;オクターブに比例してデチューン量を加重する
.detune_loop:
		jsr	detune_write_sub
		dec	<drvtmp0
		bne	.detune_loop
	.endif
	rts

.frequency_table:
	db	$cc,$3e,$02,$00	; c	$023ECB
	db	$fa,$60,$02,$00	; c+	$0260F7
	db	$30,$85,$02,$00	; d	$02852B
	db	$8d,$ab,$02,$00	; d+	$02AB8B
	db	$33,$d4,$02,$00	; e	$02D432
	db	$43,$ff,$02,$00	; f	$02FF42
	db	$e2,$2c,$03,$00	; f+	$032CE3
	db	$39,$5d,$03,$00	; g	$035D39
	db	$6e,$90,$03,$00	; g+	$039068
	db	$b0,$c6,$03,$00	; a	$03C6B0
	db	$34,$00,$04,$00	; a+	$040034
	db	$1b,$3d,$04,$00	; b	$043D1B

;
;              n(周波数用データ) * 440 * (2-F)              4
; 再生周波数 = -------------------------------   *  ----------------
;                           15467                   ch(チャンネル数)
;
; n : 再生周波数用データは18bitで構成される$0-$3FFFF
; F : オクターブ( $44 , $4c ,... の第4ビット)0で１オクターブ上がる
;ch : 使用チャンネル数 1-8
;
;o1a =   1933 =   78Dh = 000000011110001101
;o4a =  15467 =  3C6Bh = 000011110001101011
;o8a = 247472 = 3C6B0h = 111100011010110000
;
;o8a より高い音は出ません（テーブルはo8のモノ）
;ピッチベンドもLFOも　ｘ　オクターブにすれば大体115を指定すると次の音だなぁ
;良い具合になるな。やっぱそうするかなぁ〜


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_n106_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_n106_read:
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
	bne	.bank_command
	jsr	loop_sub2
	jmp	.next_cmd

;----------
;バンク切り替え
.bank_command:
	cmp	#CMD_BANK_SWITCH
	bne	.slur
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
;スラー
.slur:
	cmp	#CMD_SLUR
	bne	.wave_set
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	.next_cmd

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

	;バンクをN163音色テーブルに切り替える
	lda	#bank(n106_wave_init)*2
	jsr	change_bank

	;波形データ長(L)を読み込み、2ビット左シフトしてtemporaryへ
	;Lは実データ長ではなく、レジスタへ設定する値
	lda	n106_wave_init,x
	asl	a
	asl	a
	sta	temporary

	;波形データソース開始アドレスをtemp_data_add(16bit)に格納する
	lda	n106_wave_table,x
	sta	<temp_data_add
	inx
	lda	n106_wave_table,x
	sta	<temp_data_add+1

	;波形データオフセット(ニブル単位)を読み込み、スタックに積む
	;XXX:レジスタにはニブル単位で設定できるが、このドライバは
	;バイト境界でしか扱えないため、常にLSBは0でないと齟齬が出る
	lda	n106_wave_init,x
	pha

	;波形長の設定
	;$7C - LLLLLLFF
	;      ||||||||
	;      ||||||++-- 分周器の最上位2ビット
	;      ++++++---- 波形長 (64-L)*4 samples
	lda	#$7c
	jsr	n106_write_sub		;アドレスポートの設定
	ldx	<channel_selx2
	lda	temporary
	ora	#%11100000		;上位3bitを1で埋める(波形長4-32に制限)
	sta	n106_7c,x		;周波数設定と共用なので保存しておく
	sta	$4800			;波形データ長セット

	;Lから波形データ長(バイト単位)を算出してtemporaryへ格納
	;  L  バイト数   サンプル数
	;  0        16           32
	;  1        14           28
	;  ...
	;  7         2            4
	lsr	temporary
	lda	#$10
	sec
	sbc	temporary
	sta	temporary

	;波形データアドレスレジスタセット
	lda	#$7e			;波形データオフセットレジスタ
	jsr	n106_write_sub
	pla
	sta	$4800

	;波形データを音源内部メモリへ転送
	lsr	a			;ニブルアドレス→バイトアドレス
	ora	#%10000000		;自動インクリメントオン
	sta	$f800			;転送先の先頭アドレスを設定
	ldy	#$00
.wave_loop:
	lda	[temp_data_add],y
	sta	$4800			;データポートに波形データを書き込み
	iny
	cpy	temporary		;temporary=データ長(バイト単位)
	bmi	.wave_loop

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

	sta	temporary
	bpl	.softenve_part		;ソフトエンベ処理へ

	;音量直接指定
.volume_part:
	lda	effect_flag,x
	and	#~EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00001111
	sta	n106_volume,x

	lda	#$7f			;$7F : 音量レジスタ
	jsr	n106_write_sub
	lda	n106_7f			;ch1の上位2bitは総チャンネル数なので
	ora	n106_volume,x		;音量と重畳する。ch1以外では単に無視
	sta	$4800

	jsr	sound_data_address
	jmp	.next_cmd

	;ソフトエンベ有効化
.softenve_part:
	jsr	volume_sub
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
	lda	#$7f			;$7F : 音量レジスタ
	jsr	n106_write_sub
	lda	n106_7f			;音量0 + 総チャンネル数重畳
	sta	$4800

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
	bne	.shift_amount_set
	jsr	wait_sub
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ピッチシフト量設定
.shift_amount_set:
	cmp	#CMD_PITCH_SHIFT_AMOUNT
	bne	.keyon_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	pitch_shift_amount,x
	jsr	sound_data_address
	jmp	.next_cmd

;----------
;キーオンコマンド
.keyon_set:
	;XXX:知らないコマンドが来たときの処理はあったほうが良いかも
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	n106_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	.no_slur
	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on
.no_slur:
	lda	#$7f			;$7F - 音量レジスタ
	jsr	n106_write_sub
	lda	n106_7f
	ora	n106_volume,x
	sta	$4800
	jsr	effect_init
	rts				;音長を伴うコマンドなのでこのまま終了


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; sound_n106_write : 分周器レジスタへ書き込む
;
; 入力:
;	sound_freq_{low,high,n106},x : 分周器レジスタの値
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	t0 : 破壊
;	音源 : 反映
; 備考:
;	XXX: サブルーチン名
;
sound_n106_write:
	ldx	<channel_selx2

	lda	#$78
	jsr	n106_write_sub
	lda	sound_freq_low,x
	sta	$4800

	lda	#$7a
	jsr	n106_write_sub
	lda	sound_freq_high,x
	sta	$4800

	lda	#$7c
	jsr	n106_write_sub
	lda	n106_7c,x		;波形長を重畳
	ora	sound_freq_n106,x
	sta	$4800
	rts


;--------------------
; n106_write_sub : 各チャンネルに対応したアドレスポートを設定
;
; 入力:
;	a : 書き込むアドレスのオフセット
;	chennel_sel : 現在の(グローバルな)チャンネル番号
; 副作用:
;	a : 破壊
;	t0 : 破壊
;	N163アドレスポート : a - ch*8
;
n106_write_sub:
	;t0へオフセットを退避
	;XXX:pha/pla使おう
	sta	<t0

	;ch*8の二の補数を取り、t0を足す
	lda	<channel_sel
	sec
	sbc	#PTRN106	;N163のチャンネル番号に直す
	asl	a
	asl	a
	asl	a
	eor	#$ff		;一の補数
	sec			;次の和で+1することによって二の補数とする
	adc	<t0
	sta	$f800		;アドレスポート
	rts


;-------------------------------------------------------------------------------
;各エフェクトのフレーム処理サブルーチン
;-------------------------------------------------------------------------------

;--------------------
;sound_n106_softenve : ソフトウェアエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	y : 破壊
;	register_low,x : 反映
;	音量 : 反映
;	(以下volume_enve_subからの間接的な副作用)
;	soft_add_{low,high},x : 反映
;	バンク : softenve_tableのあるバンク
; 備考:
;	XXX:サブルーチン名
;
sound_n106_softenve:
	jsr	volume_enve_sub
	sta	temporary
	lda	#$7f			;$7F - 音量レジスタ
	jsr	n106_write_sub
	lda	n106_7f			;チャンネル数ビットを重畳
	ora	temporary
	sta	$4800
	jmp	enverope_address	;アドレス1個増やす


;--------------------
; sound_n106_lfo : ピッチLFOのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	y : 破壊
;	音程 : 反映
;	(以下lfo_subからの間接的な副作用)
;	sound_freq_{low,high,n106},x : 反映
;	lfo_start_counter,x : 反映
;	lfo_reverse_counter,x : 反映
;	lfo_adc_sbc_counter,x : 反映
;	effect_flag,x : EFF_SOFTLFO_DIRビットが影響を受ける
; 備考:
;	XXX:サブルーチン名
;
sound_n106_lfo:
	jsr	lfo_sub
	jmp	sound_n106_write


;--------------------
; sound_n106_pitch_enve : ピッチエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	y : 破壊
;	pitch_add_{low,high},x : 反映
;	sound_freq_{low,high},x : 反映
;	音程 : 反映
;	(以下pitch_subからの間接的な副作用)
;	バンク : #bank(pitchenve_table)
; 備考:
;	XXX:サブルーチン名
;
sound_n106_pitch_enve:
	jsr	pitch_sub
	jsr	sound_n106_write
	jmp	pitch_enverope_address	;アドレス1個増やす


;--------------------
; sound_n106_note_enve : ノートエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	y : 破壊
;	temporary2 : 破壊
;	sound_freq_{low,high},x : 反映
;	sound_lasthigh,x : 反映
;	音程 : 反映
;	arpe_add_{low,high},x : 反映
; 備考:
;	XXX:サブルーチン名
;
sound_n106_note_enve:
	jsr	note_enve_sub
	bcs	.done			;0なので書かなくてよし
	jsr	n106_freq_set
	jsr	sound_n106_write
	jsr	arpeggio_address	;アドレス1個増やす
	rts
.done
;	jsr	n106_freq_set
	jsr	arpeggio_address	;アドレス1個増やす
	rts
