;-----------------------------------------------------------------------
;VRC7 sound driver
;-----------------------------------------------------------------------

;
;I/Oポートアドレス
; - アドレスレジスタに書き込んだ後は6clkのウェイトが必要
; - データレジスタに書き込んだ後は42clkのウェイトが必要
;
VRC7_ADDR_PORT = $9010		;アドレスレジスタ
VRC7_DATA_PORT = $9030		;データレジスタ

;
;FM音源(OPLLカスタム)レジスタマップ
;
; $00-$07 : ユーザ定義音色
;   $00 : モジュレータ
;   $01 : キャリア
;	APSKMMMM
;	||||||||
;	||||++++--- マルチプル(0:1/2, 1:1, 2:2 ....)
;	|||+------- キーレートスケーリング
;	||+-------- サスティーン(1:有効)
;	|+--------- ピッチLFO(1:有効)
;	+---------- アンプLFO(1:有効)
;   $02 : モジュレータ
;	KKOOOOOO
;	||||||||
;	||++++++--- モジュレータレベル(0:0dB, 1:-0.75dB, ...)
;	++--------- モジュレータキーレベルスケーリング
;   $03 : キャリア/モジュレータ
;	KK-WwFFF
;	|| |||||
;	|| ||+++--- モジュレータフィードバック
;	|| |+------ モジュレータ波形(0:正弦波, 1:正弦波の0以下をクリップ)
;	|| +------- キャリア波形(同上)
;	++--------- キャリアキーレベルスケーリング
;   $04 : モジュレータ
;   $05 : キャリア
;	AAAADDDD
;	||||||||
;	||||++++--- ディケイレート
;	++++------- アタックレート
;   $06 : モジュレータ
;   $07 : キャリア
;	SSSSRRRR
;	||||||||
;	||||++++--- リリースレート
;	++++------- サスティーンレベル(0:0dB, 1:-3dB, ...)
;
; $10-$15 : 分周器
;	LLLLLLLL
;	||||||||
;	++++++++--- 分周器の下位8bit
;
; $20-$25 : 分周器、オクターブ、トリガ、サスティーン
;	--STOOOH
;	  ||||||
;	  |||||+--- 分周器の最上位ビット
;	  ||+++---- オクターブプリスケーラ
;	  |+------- トリガ(このビットが0→1されるとキーオン、1→0されるとオフ)
;	  +-------- サスティーン
;
; freq = (49716 * HLLLLLLLL) / 2^(19 - OOO) [Hz]
;
; $30-$35 : 音量、音色
;	IIIIVVVV
;	||||||||
;	||||++++--- 音量(0:0dB, 1:-3dB ...)
;	++++------- 音色番号(0:ユーザ定義音色, 1-15:プリセット)
;
VRC7_REG_FNUM_LOW = $10
VRC7_REG_FNUM_HI  = $20
VRC7_REG_INST_VOL = $30


;--------------------
; sound_vrc7 : NMI割り込みエントリポイント
;
; 備考:
;	XXX:サブルーチン名
;
sound_vrc7:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	vrc7_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go:
	jsr	sound_vrc7_read
	jsr	vrc7_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	jsr	sound_vrc7_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done:
	rts


;--------------------
; vrc7_do_effect : 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
vrc7_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	;VRC7にはデューティー比エンベロープは無い

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_vrc7_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_vrc7_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_vrc7_pitch_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.done
	;同一フレームのsound_vrc6_readの処理でキーオンが行われたかどうかで
	;ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	;キーオンが行われてないフレームは通常の処理
	jsr	sound_vrc7_note_enve
	jmp	.done
.arpe_key_on:
	;キーオンが行われたフレームはワークエリアの調整のみ行う
	;実際にレジスタに反映するのは sound_vrc7 の最後
	jsr	note_enve_sub
	jsr	vrc7_freq_set
	jsr	arpeggio_address
.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; vrc7_freq_set : ノート番号を周波数データに変換し、キーオンフラグを立てる
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
;	temporary : 破壊
;	vrc7_key_stat,x : 反映
; 備考:
;	このサブルーチンは音源レジスタへの書き込みは行わない
;	XXX:内蔵音源と共通化可能っぽい
;	XXX:キーオンフラグをここで立てるのが良いのか不明
;	XXX:もっと最適化できそう
;
vrc7_freq_set:
	ldx	<channel_selx2

	;音階→周波数変換テーブルのオフセット計算
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a			;2倍してワードオフセットにする
	tay

	lda	sound_sel,x		;音階データ読み出し
	and	#%11110000		;オクターブ
					;XXX:結果を捨てるのでandは不要
	;注:carryはaslでクリアされている
	adc	#$90			;7 + 9 = 0x10
	bcs	.freq_normal

	;オクターブ0-6
	;オクターブプリスケーラを+1する代わりに分周器を1/2にする
	;XXX:精度と引き換えにデチューン幅を確保してる模様。
	;    (2004/06/15 improve too strong detune. というコメントがあった)
	;    N163と同様にSAコマンドを活用すべきな気も。
	;XXX:いっそテーブル2つ用意したほうがいいのでは
.freq_half:
	lda	.freq_table+1,y		;周波数テーブルからMSBを読み出す
	lsr	a			;2で割る
	sta	sound_freq_high,x	;書き込み

	lda	.freq_table,y		;周波数テーブルから下位8bitを読み出す
	ror	a			;2で割る
	sta	sound_freq_low,x	;書き込み
	jmp	.oct_set

	;オクターブ7-8
.freq_normal:
	lda	.freq_table+1,y		;周波数テーブルからMSBを読み出す
	sta	sound_freq_high,x	;書き込み

	lda	.freq_table,y		;周波数テーブルから下位8bitを読み出す
	sta	sound_freq_low,x	;書き込み

	;オクターブプリスケーラの設定
.oct_set:
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	;オクターブ0-7ならオクターブプリスケーラを+1する
	;XXX:cmp #$7 / bcs .skip / sbc #0 / .skip: でいいんじゃなかろうか
	clc
	adc	#$01
	cmp	#$8
	bcc	.oct_under_7
	sbc	#$01
.oct_under_7:
	and	#%00000111		;3bitのみ利用可
	asl	a			;オクターブのビット位置に移動
	sta	temporary		;temporaryに退避

	;vrc7_key_stat,xのキーオンフラグを立て、オクターブを設定する
	;XXX:temporaryでなくゼロページのdrvtmp0あたりを使えば最適化可能
	lda	vrc7_key_stat,x
	and	#%11110001
	ora	#%00010000
	sta	vrc7_key_stat,x
	lda	temporary
	ora	vrc7_key_stat,x
	sta	vrc7_key_stat,x

	jsr	detune_write_sub

	rts

.freq_table:
	dw	$00AC, $00B6, $00C1, $00CD	; c  c+ d  d+
	dw	$00D9, $00E6, $00F3, $0102	; e  f  f+ g
	dw	$0111, $0121, $0133, $0145	; g+ a  a+ b
	;XXX:12-15


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_vrc7_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_vrc7_read:
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
.bank_set
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
;XXX:サブルーチン化
.wave_set:
	cmp	#CMD_TONE
	bne	.volume_set

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary

	;64-127ならユーザ定義音色のストアを行う
	;実際にこの音色を使うには、別途チャンネルごとに音色0を設定する必要がある
	;ユーザ定義音色は全てのチャンネルを通して同時に1つしかストアできない
	;同じ音色を複数のチャンネルで共用することは可能
	and	#%01000000
	cmp	#%01000000
	bne	.set_tone

	;ユーザ定義音色の転送
	lda	temporary
	and	#$3F
	asl	a
	tax

	;定義バンク切り替え
	lda	#bank(vrc7_data_table)*2
	jsr	change_bank

	;temp_data_addに音色データのアドレスを格納する
	lda	vrc7_data_table,x
	sta	<temp_data_add
	inx				;XXX:inxじゃなくて
	lda	vrc7_data_table,x	;    vrc7_data_table+1でいいのでは
	sta	<temp_data_add+1

	;FMアドレス$00-$07に音色データを転送する
	ldy	#$8			;ループ回数
	sty	temporary

	ldy	#$00			;y : FMアドレス / データオフセット
.loop_set_fmdata:
	sty	VRC7_ADDR_PORT
	jsr	vrc7_write_reg_wait2
	lda	[temp_data_add],y
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait
	iny
	cpy	temporary		;XXX:なんでimmediateにしないのか
	bmi	.loop_set_fmdata

	jmp	.end_tone_set

.set_tone:
	;音色番号をボリュームレジスタの上位4ビットに設定する
	lda	temporary		;音色番号
	asl	a
	asl	a
	asl	a
	asl	a
	sta	temporary
	lda	vrc7_volume,x
	and	#%00001111
	ora	temporary
	sta	vrc7_volume,x

	;実際にFMレジスタへ設定する
	lda	#VRC7_REG_INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f			;FMは音量の増減が逆なので反転する
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait

.end_tone_set:
	ldx	<channel_selx2
	jsr	sound_data_address
	jmp	.next_cmd

;----------
;音量設定
;XXX:サブルーチン化
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

	;下位4bit:音量 / 上位4bit:音色
	lda	temporary
	and	#%00001111
	sta	temporary

	lda	vrc7_volume,x
	and	#%11110000
	ora	temporary
	sta	vrc7_volume,x

	;レジスタライト
	lda	#VRC7_REG_INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f			;FMは音量の増減が逆なので反転する
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait

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

	lda	rest_flag,x
	ora	#RESTF_REST
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	jsr	.key_off

	jsr	sound_data_address
	rts				;音長を伴うコマンドなのでこのまま終了

;-------------
;キーオフサブルーチン
.key_off
	lda	vrc7_key_stat,x
	and	#%11101111
	sta	temporary

	lda	#VRC7_REG_FNUM_HI
	jsr	vrc7_adrs_ch
	lda	sound_freq_high,x
	ora	temporary
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait

	rts

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
	bne	.slur
	jsr	wait_sub
	rts				;音長を伴うコマンドなのでこのまま終了

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
.keyon_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	vrc7_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#EFF2_SLUR_ENABLE
	beq	.no_slur

	;スラー時の処理
	lda	effect2_flags,x
	and	#~EFF2_SLUR_ENABLE
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

.no_slur:
	;音量の再設定
	lda	#VRC7_REG_INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f			;FMは音量の増減が逆なので反転する
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait
	;最後にキーオフをしておく
	;このフレームの処理の最後で再びキーオンされるので、
	;結果としてスラーではなくなる
	jsr	.key_off
	jsr	effect_init
	rts				;音長を伴うコマンドなのでこのまま終了


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; sound_vrc7_write : 分周器レジスタへ書き込む
;
; 入力:
;	sound_freq_{low,high},x : 分周器の設定値
;	vrc7_key_stat,x : オクターブプリスケーラやキーオンフラグなど
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	音源 : 反映
; 備考:
;	XXX: サブルーチン名
;
sound_vrc7_write:
	ldx	<channel_selx2

	;下位
	lda	#VRC7_REG_FNUM_LOW
	jsr	vrc7_adrs_ch
	lda	sound_freq_low,x
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait

	;上位
	lda	#VRC7_REG_FNUM_HI
	jsr	vrc7_adrs_ch
	lda	sound_freq_high,x
	ora	vrc7_key_stat,x
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait

	rts

;--------------------
; vrc7_adrs_ch - FMアドレスレジスタにチャンネル番号を反映したアドレスを書き込む
;
; 入力:
;	a : レジスタオフセット
;	channel_sel : チャンネル番号
; 副作用:
;	FMアドレスレジスタ : 反映
;	t0 : 破壊
; 備考:
;	アドレスライト後に適切なウェイト(6clk以上)が挿入される
;	XXX:サブルーチン名
;
vrc7_adrs_ch:
	sta	<t0
	lda	<channel_sel
	sec
	sbc	#PTRVRC7
	ora	<t0
	sta	VRC7_ADDR_PORT
	;6clkのウェイトを入れる
	;XXX:rtsに6clkかかるのでnopいらないはずだが
	nop			;2clk
	nop			;2clk
	nop			;2clk
	rts


;--------------------
; vrc7_write_reg_wait : FMデータレジスタライト後のウェイト
;
; 備考:
;	42clk以上のウェイトが必要
;
vrc7_write_reg_wait:
	pha			;3clk
	lda	#$01		;2clk

	;ループ:(2+3)*7 + 2+2 = 39clk
.wait:
	asl	a		;2clk
	bcc	.wait		;分岐時3clk / 通過時2clk

	pla			;4clk
	;fallthrough

;--------------------
; vrc7_write_reg_wait2 : FMアドレスレジスタライト後のウェイト
;
; 備考:
;	6clk以上のウェイトが必要
;	XXX:jsrが3バイト6clkなので、単にnop三つをマクロで挿入したほうが良い
;
vrc7_write_reg_wait2:
	rts			;6clk


;-------------------------------------------------------------------------------
;各エフェクトのフレーム処理サブルーチン
;-------------------------------------------------------------------------------

;--------------------
;sound_vrc7_softenve : ソフトウェアエンベロープのフレーム処理
;
; 入力:
; 副作用:
;	a : 破壊
;	temporary : 破壊
;	音量 : 反映
;	(以下volume_enve_subからの間接的な副作用)
;	x : channel_selx2になる
;	y : 破壊
;	バンク : softenve_tableのあるバンク
;	soft_add_{low,high},x : リピートマークを指していた場合には先頭に戻る
;	(以下vrc7_adrs_chからの間接的な副作用)
;	t0 : 破壊
;	(以下volume_enve_subからの間接的な副作用)
;	soft_add_{low,high},x : 反映
;	バンク : softenve_tableのあるバンク
; 備考:
;	XXX:サブルーチン名
;
sound_vrc7_softenve:
	jsr	volume_enve_sub
	sta	temporary
	lda	#VRC7_REG_INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	and	#%11110000
	ora	temporary
	eor	#$0f			;FMは音量の増減が逆なので反転する
	sta	VRC7_DATA_PORT
	jsr	vrc7_write_reg_wait
	jmp	enverope_address


;--------------------
; sound_vrc7_lfo : ピッチLFOのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	音程 : 反映
;	(以下sound_vrc7_writeからの間接的な副作用)
;	a : 破壊
;	(以下lfo_subからの間接的な副作用)
;	sound_freq_{low,high},x : 反映
;	lfo_start_counter,x : 反映
;	lfo_reverse_counter,x : 反映
;	lfo_adc_sbc_counter,x : 反映
;	effect_flag,x : EFF_SOFTLFO_DIRビットが影響を受ける
; 備考:
;	XXX:サブルーチン名
;
sound_vrc7_lfo:
	jsr	lfo_sub
	jmp	sound_vrc7_write


;--------------------
; sound_vrc7_pitch_enve : ピッチエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	sound_freq_{low,high},x : 反映
;	音程 : 反映
;	(以下sound_vrc7_writeからの間接的な副作用)
;	a : 破壊
;	(以下pitch_subからの間接的な副作用)
;	pitch_add_{low,high},x : 反映
;	バンク : #bank(pitchenve_table)
;	(以下pitch_enverope_addressからの間接的な副作用)
;	y : 破壊
;	temporary : 破壊
; 備考:
;	XXX:サブルーチン名
;
sound_vrc7_pitch_enve:
	jsr	pitch_sub
	jsr	sound_vrc7_write
	jmp	pitch_enverope_address


;--------------------
; sound_vrc7_note_enve : ノートエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	sound_freq_{low,high},x : 反映
;	音程 : 反映
;	(以下note_enve_subからの間接的な副作用)
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
;	t0 : 破壊
;	arpe_add_{low,higi},x : リピートマークを指していた場合には先頭に戻る
;	バンク : arpeggio_tableのあるバンク
;	(以下vrc7_freq_setからの間接的な副作用)
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
;	temporary : 破壊
;	vrc7_key_stat,x : 反映
;	(以下arpeggio_addressからの間接的な副作用)
;	arpe_add_{low,high},x : 反映
; 備考:
;	XXX:サブルーチン名
;
sound_vrc7_note_enve:
	jsr	note_enve_sub
	bcs	.done			;0なので書かなくてよし
	jsr	vrc7_freq_set
	jsr	sound_vrc7_write
	jsr	arpeggio_address
	rts
.done:
;	jsr	vrc7_freq_set
	jsr	arpeggio_address
	rts
