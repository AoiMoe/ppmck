;-----------------------------------------------------------------------
;MMC5 driver
;-----------------------------------------------------------------------

MMC5_REG_CTRL	=	$5000	; コントロールレジスタ
MMC5_REG_FREQ_L	=	$5002	; 周波数(L)レジスタ
MMC5_REG_FREQ_H	=	$5003	; 周波数(H)レジスタ
MMC5_START_CH	=	PTRMMC5	; 開始ch


;--------------------
; mmc5_sound_init - MMC5の初期化
;
; 副作用:
;	a : 破壊
;	MMC5 : 初期化
;
mmc5_sound_init:
	lda	#$03
	sta	$5015
	rts


;--------------------
; sound_mmc5 - NMI割り込みエントリポイント(MMC5)
;
; 備考:
;	XXX:サブルーチン名
;
sound_mmc5:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	mmc5_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go:
	jsr	sound_mmc5_read
	jsr	mmc5_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	jsr	sound_mmc5_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done
	rts


;--------------------
; mmc5_do_effect - 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
mmc5_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	lda	effect_flag,x
	and	#EFF_DUTYENV_ENABLE
	beq	.enve_write
	jsr	sound_mmc5_dutyenve

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_mmc5_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_mmc5_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_mmc5_pitch_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.done
	;同一フレームのsound_mmc5_readの処理でキーオンが行われたかどうかで
	;ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	;キーオンが行われてないフレームは通常の処理
	jsr	sound_mmc5_note_enve
	jmp	.done
.arpe_key_on:
	;キーオンが行われたフレームはワークエリアの調整のみ行う
	;実際にレジスタに反映するのは sound_mmc5 の最後
	jsr	note_enve_sub
	jsr	mmc5_freq_set
	jsr	arpeggio_address

.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; mmc5_freq_set : ノート番号を周波数データに変換する
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
;	XXX:内蔵音源と共通化可能っぽい
;
mmc5_freq_set:
	ldx	<channel_selx2

	;音階→周波数変換テーブルのオフセット計算
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	lda	psg_frequency_table,y	;PSG周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	psg_frequency_table+1,y	;PSG周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

	;オクターブ処理
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;

	;内蔵音源との実オクターブ差の調整
	sec
	sbc	#$02

	;XXX:オクターブ値が負になった場合のチェックが必要
	beq	.done			;ゼロならそのまま終わり

	tay				;y=オクターブ値(ループ回数)
.oct_loop:
	;1オクターブ上がるごとに分周器の設定値を1/2する
	lsr	sound_freq_high,x	;符号なし16bit右シフト
	ror	sound_freq_low,x	;
	dey
	bne	.oct_loop		;オクターブ分繰り返す

.done:
	jsr	detune_write_sub
	rts


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_mmc5_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_mmc5_read:
.next_cmd:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]

;----------
;ループ処理1
.loop_program
	cmp	#CMD_LOOP1
	bne	.loop_program2
	jsr	loop_sub
	jmp	.next_cmd

;----------
;ループ処理2(分岐)
.loop_program2
	cmp	#CMD_LOOP2
	bne	.bank_command
	jsr	loop_sub2
	jmp	.next_cmd

;----------
;バンク切り替え
.bank_command
	cmp	#CMD_BANK_SWITCH
	bne	.duty_set
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
.duty_set:
	cmp	#CMD_TONE
	bne	.volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	.duty_enverope_part	;ヂューティエンベ処理へ

	;デューティー比直接指定
	;register_high = 上位4bitを一時退避先として利用
.duty_select_part:
	lda	effect_flag,x
	and	#~EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ無効指定

	lda	effect2_flags,x         ; hw_envelope
	and	#EFF2_HWENV_MASK
	eor	#EFF2_HWENV_MASK
	sta	register_high,x

	pla
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a

;;;;	ora	#EFF2_HWENV_MASK	;waveform hold on & hardware envelope off

	ora	register_high,x
	sta	register_high,x		;書き込み
	ora	register_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y
	jsr	sound_data_address
	jmp	.next_cmd

	;デューティーエンベロープの指定
.duty_enverope_part:
	lda	effect_flag,x
	ora	#EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;デューティエンベロープ有効指定
	pla
	sta	duty_sel,x
	asl	a
	tay

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	lda	dutyenve_table,y	;デューティエンベロープアドレス設定
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
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
	sta	register_low,x
	ora	register_high,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y			;ボリューム書き込み
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
	lda	register_high,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y

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
	bne	.hwenv
	jsr	wait_sub
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ハードエンベロープ
.hwenv:
	cmp	#CMD_HWENV
	bne	.slur

	jsr	sound_data_address
	lda	effect2_flags,x
	and	#~EFF2_HWENV_MASK
	ora	[sound_add_low,x]
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	.next_cmd

;----------
;スラー
.slur:
	cmp	#CMD_SLUR
	bne	.keyon_set
	lda	effect2_flags,x
	ora	#EFF2_SLUR_ENABLE
	sta	effect2_flags,x
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
	jsr	mmc5_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	.no_slur
	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

.no_slur:
	jmp	effect_init


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; sound_mmc5_write - 音量レジスタおよび分周器レジスタへ書き込む
;
; 入力:
;	channel_selx2 :
;	channel_selx4 :
;	register_{low,high},x : 音量レジスタの値
;	sound_freq_{low,high},x : 分周器レジスタの値
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	sound_lasthigh,x : 分周器レジスタの上位8bitの現在値を反映
;	音源 : 反映
; 備考:
;	XXX: サブルーチン名
;
sound_mmc5_write:
	ldx	<channel_selx2
	ldy	<channel_selx4

	;音量レジスタ
	lda	register_low,x		;音量
	ora	register_high,x		;デューティー比など
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y

	;分周器
	lda	sound_freq_low,x	;Low Write
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x	;High Write
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y

	rts


;-------------------------------------------------------------------------------
;各エフェクトのフレーム処理サブルーチン
;-------------------------------------------------------------------------------

;--------------------
;sound_mmc5_softenve : ソフトウェアエンベロープのフレーム処理
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
sound_mmc5_softenve:
	jsr	volume_enve_sub
	sta	register_low,x
	ora	register_high,x		;音色データ（上位4bit）と音量（下位4bit）を合成
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y	;書き込み
	jsr	enverope_address	;アドレス一個増やして
	rts				;おしまい


;--------------------
; sound_mmc5_lfo : ピッチLFOのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	y : 破壊
;	temporary : 破壊
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
sound_mmc5_lfo:
	lda	sound_freq_high,x
	sta	temporary
	jsr	lfo_sub
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
.done:
	rts


;--------------------
; sound_mmc5_pitch_enve : ピッチエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	y : 破壊
;	temporary : 破壊
;	pitch_add_{low,high},x : 反映
;	sound_freq_{low,high},x : 反映
;	音程 : 反映
;	(以下pitch_subからの間接的な副作用)
;	バンク : #bank(pitchenve_table)
; 備考:
;	XXX:サブルーチン名
;
sound_mmc5_pitch_enve:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub

	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
.done:
	jsr	pitch_enverope_address	;アドレス一個増やす
	rts


;--------------------
; sound_mmc5_note_enve : ノートエンベロープのフレーム処理
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
sound_mmc5_note_enve:
;	lda	sound_freq_high,x
;	sta	temporary2
	jsr	note_enve_sub
	bcs	.done			;0なので書かなくてよし

	jsr	mmc5_freq_set
	ldx	<channel_selx2
	ldy	<channel_selx4
	lda	sound_freq_low,x
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
;	cmp	temporary2
;	beq	.done2
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
;.done2:
	jsr	arpeggio_address
	rts

.done:
;	jsr	mmc5_freq_set
	jsr	arpeggio_address
	rts


;--------------------
; sound_mmc5_dutyenve : デューティー比エンベロープのフレーム処理
;
; 副作用:
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
;	duty_add_{low,high},x : 反映
;	音色 : 反映
;	register_high,x : 反映
;	バンク : dutyenve_tableのあるバンク
; 備考:
;	XXX:サブルーチン名
;
sound_mmc5_dutyenve:
	ldx	<channel_selx2

	; 定義バンク切り替え
	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	indirect_lda	duty_add_low	;エンベロープデータ読み込み
	cmp	#$ff			;末尾かどーか
	beq	.do_repeat		;末尾ならリピート処理へ

	; ハードウェアエンベロープをregister_high,xへ反映
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	ora	#EFF2_HWENV_MASK	;waveform hold on & hardware envelope off
	sta	register_high,x

	ora	register_low,x		;音色データ（上位4bit）と音量（下位4bit）を合成
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y	;書き込み
	jsr	duty_enverope_address	;アドレス一個増やす
	rts

.do_repeat:
	; リピートテーブルを参照してデータアドレスをリストアする
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	sound_mmc5_dutyenve
