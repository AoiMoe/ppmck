;-----------------------------------------------------------------------
;Sunsoft 5B driver (Gimmick!) - superset of Sunsoft FME-7 (no sound).
;
;Sunsoft 5B supports AY 3-8910 compatible noise and envelope generator.
;
;C000 :AY 3-8910 address(W)
;E000 :AY 3-8910 data(W) (original AY 3-8910 can be read, too)
;
;Reference: http://www.howell1964.freeserve.co.uk/parts/ay3891x_datasheet.htm
;           http://www.breezer.demon.co.uk/spec/tech/ay-3-8912.html etc...
;
;AY 3-8910 Registers
;00	Ch. A freq data lower bits
;	FFFFFFFF
;01	Ch. A freq data higher bits
;	----FFFF
;	freq=1.79/(F*16) Mhz
;
;
;02	Ch. B freq data lower bits
;03	Ch. B freq data higher bits
;04	Ch. C freq data lower bits
;05	Ch. C freq data higher bits
;
;
;06	Noise pitch
;	---FFFFF
;	freq=1.79/(F*16) Mhz
;
;
;07	Mixer
;	0:Enable 1:Disable
;	IINNNTTT
;	||||||||
;	|||||||+-------- Ch A Tone
;	||||||+--------- Ch B Tone
;	|||||+---------- Ch C Tone
;	||||+----------- Ch A Noise
;	|||+------------ Ch B Noise
;	||+------------- Ch C Noise
;	|+-------------- (GPIO A direction - unused)
;	+--------------- (GPIO B direction - unused)
;
;
;08	Ch. A volume
;	---MVVVV
;	V: Volume
;	M: Mode (1: Use envelope, 0:Use fixed volume)
;
;
;09	Ch. B volume
;0a	Ch. C volume
;
;
;0b	Envelope duration lower bits
;	DDDDDDDD
;0c	Envelope duration higher bits
;	DDDDDDDD
;	freq = 1.79/(D*256) Mhz
;	(duration = D*256/1.79 sec)
;
;
;0d	Envelope shape
;	----CAAH
;	    ||||
;	    |||+-------- Hold
;	    ||+--------- Alternate
;	    |+---------- Atack
;	    +----------- Continue
;
;
;0e	GPIO A (unused)
;0f	GPIO B (unused)
;-----------------------------------------------------------------------

FME7_ADDR	=	$C000
FME7_DATA	=	$E000


;--------------------
; fme7_sound_init : Sunsoft 5Bの初期化
;
; 副作用:
;	a : 破壊
;	音源 : 初期化
;
fme7_sound_init:
	;音量(レジスタ$8-$A)を0に
	ldy	#$0A
	lda	#$00
.loop:
	sty	FME7_ADDR
	sta	FME7_DATA
	dey
	cpy	#$07
	bne	.loop

	;各チャンネルのtoneを許可、ノイズを不許可
	lda	#%11111000
	sty	FME7_ADDR
	sta	FME7_DATA
	sta	fme7_reg7
	rts


;--------------------
; fme7_dst_adr_set : チャンネルに対応するオフセットを計算する
;
; 入力:
;	channel_sel : 現在のグローバルなチャンネル番号
; 出力:
;	fme7_ch_sel : Sunsoft 5Bチャンネル番号(0-2)
;	fme7_ch_selx2 : fme7_ch_sel * 2
;	fme7_ch_selx4 : fme7_ch_sel * 4
;	fme7_vol_regno : 音量レジスタ番号(8-10)
; 副作用:
;	a : 破壊
;
fme7_dst_adr_set:
	lda	<channel_sel
	sec
	sbc	#PTRFME7		;FME7の何チャンネル目か？
	sta	fme7_ch_sel
	asl	a
	sta	fme7_ch_selx2
	asl	a
	sta	fme7_ch_selx4
	lda	fme7_ch_sel
	clc
	adc	#$08
	sta	fme7_vol_regno
	rts


;--------------------
; sound_fme7 : NMI割り込みエントリポイント
;
; 備考:
;	XXX:サブルーチン名
;
sound_fme7:
	;チャンネル番号チェック。
	;XXX:不要だと思う
	lda	<channel_sel
	cmp	#PTRFME7+3
	beq	.done

	jsr	fme7_dst_adr_set

	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	fme7_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go
	jsr	sound_fme7_read
	jsr	fme7_do_effect
	lda	rest_flag,x
	and	#RESTF_KEYON		;キーオンフラグ
	beq	.done
	;キーオンフラグが立っていたらデータ書き出し
	jsr	fme7_write_volume_and_freq
	lda	rest_flag,x
	and	#~RESTF_KEYON		;キーオンフラグオフ
	sta	rest_flag,x
.done:
	rts


;--------------------
; fme7_do_effect : 各エフェクトのフレーム処理
;
; 入力:
;	x : channel_selx2
; 備考:
;	XXX:サブルーチン名
;	XXX:共通化
;
fme7_do_effect:
	lda	rest_flag,x
	and	#RESTF_REST
	beq	.duty_write
	rts				;休符なら終わり

.duty_write:
	;Sunsoft 5Bにデューティー比は無い

.enve_write:
	lda	effect_flag,x
	and	#EFF_SOFTENV_ENABLE
	beq	.lfo_write
	jsr	sound_fme7_softenve

.lfo_write:
	lda	effect_flag,x
	and	#EFF_SOFTLFO_ENABLE
	beq	.pitchenve_write
	jsr	sound_fme7_lfo

.pitchenve_write:
	lda	effect_flag,x
	and	#EFF_PITCHENV_ENABLE
	beq	.toneenve_write
	jsr	sound_fme7_pitch_enve

.toneenve_write:
	lda	effect_flag,x
	and	#EFF_DUTYENV_ENABLE
	beq	.arpeggio_write
	jsr	sound_fme7_tone_enve

.arpeggio_write:
	lda	effect_flag,x
	and	#EFF_NOTEENV_ENABLE
	beq	.done
	;同一フレームのsound_fme7_readの処理でキーオンが行われたかどうかで
	;ノートエンベロープの処理が異なる
	lda	rest_flag,x
	and	#RESTF_KEYON
	bne	.arpe_key_on
	;キーオンが行われてないフレームは通常の処理
	jsr	sound_fme7_note_enve
	jmp	.done
.arpe_key_on:
	;キーオンが行われたフレームはワークエリアの調整のみ行う
	;実際にレジスタに反映するのは sound_fme7 の最後
	jsr	note_enve_sub
	jsr	fme7_freq_set
	jsr	arpeggio_address
.done:
	rts


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; fme7_freq_set : ノート番号を周波数データに変換する
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
fme7_freq_set:
	;トーンかノイズかのチェック
	;XXX:この音源では本来、ノイズジェネレータは全チャンネルで
	;共有されるのと、トーンとノイズが同時に出せるので、
	;内蔵音源と同じ方法でノイズ周波数を指定するのは無理があるのだが。
	ldx	<channel_selx2
	lda	fme7_tone,x
	cmp	#$02			;ノイズなら
	beq	.noise_freq_set		;飛ぶ

	;トーン周波数の処理
	;音階→周波数変換テーブルのオフセット計算
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	lda	.frequency_table,y	;Sun5B周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	.frequency_table+1,y	;Sun5B周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

	;オクターブ処理
	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	beq	.done			;ゼロならそのまま終わり

	tay
.oct_loop:
	;1オクターブ上がるごとに分周器の設定値を1/2する
	lsr	sound_freq_high,x	;符号なし16bit右シフト
	ror	sound_freq_low,x	;
	dey				;
	bne	.oct_loop		;オクターブ分繰り返す

FREQ_ROUND = 0
	;四捨五入
	.if	FREQ_ROUND
	lda	sound_freq_low,x
	adc	#$00			;上のrorからのキャリーを足す
	sta	sound_freq_low,x
	bcc	.done
	inc	sound_freq_high,x
	.endif

.done:
	jsr	detune_write_sub
	rts

	;ノイズ周波数の処理
.noise_freq_set:
	lda	sound_sel,x		;音階データ読み出し
	sta	sound_freq_low,x	;そのまま
	rts

.frequency_table:
	dw	$0D5C,$0C9D,$0BE7,$0B3C	; o0c  c+ d  d+
	dw	$0A9B,$0A02,$0973,$08EB	;   e  f  f+ g
	dw	$086B,$07F2,$0780,$0714	;   g+ a  a+ b
	dw	$0000,$0FE4,$0EFF,$0E28	; o-1  a  a+ b
; 再生周波数 = 1789772.5 / (n*32) [Hz]


;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------

;--------------------
; sound_fme7_read : 演奏データの解釈
;
; 備考:
;	XXX:音源非依存な形での共通化
;
sound_fme7_read:
__fme7_next_cmd:
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
	bne	__fme7_volume_set

	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	.tone_env_set		;音色エンベロープ処理へ

	;音色直接指定($80-$83)
	lda	effect_flag,x
	and	#~EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;音色エンベロープ無効指定

	pla
	and	#%00000011
	sta	fme7_tone,x

	jsr	fme7_tone_set_sub
	jsr	sound_data_address
	jmp	.next_cmd

.tone_env_set:
	;音色エンベロープ
	lda	effect_flag,x
	ora	#EFF_DUTYENV_ENABLE
	sta	effect_flag,x		;音色エンベロープ有効指定

	pla
	sta	duty_sel,x
	asl	a
	tay

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	lda	dutyenve_table,y	;音色エンベロープアドレス設定
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	.next_cmd

;--------------------
; fme7_tone_set_sub - 音色を音源レジスタに設定する
;
; 入力:
;	a : 音色番号(0:ミュート, 1:トーン, 2:ノイズ, 3:トーン+ノイズ)
;	fme7_ch_selx4 : fme7チャンネル番号*4
; 副作用:
;	y : 破壊
;	音源 : 反映
; 備考:
;	XXX:移動すべし
;
fme7_tone_set_sub: ; 音色セットサブルーチン(A=音色)
	;まずノイズビット、トーンビットの両方を0にする
	pha
	ldy	fme7_ch_selx4
	lda	.enable_bit_tbl,y	;@0に対応するビットパターン
	eor	#$FF			;反転するとマスクになる
	and	fme7_reg7
	sta	fme7_reg7
	pla

	clc
	adc	fme7_ch_selx4
	tay
	lda	.enable_bit_tbl,y	;ビット読み出し
	ora	fme7_reg7		;1のを1にする(Disable)
	sta	fme7_reg7		;
	ldy	#$07
	sty	FME7_ADDR
	sta	FME7_DATA
	rts

.enable_bit_tbl:
;		       @0   @1(tone)  @2(noise)   @3(both)
	db	%00001001, %00001000, %00000001, %00000000	; ch A
	db	%00010010, %00010000, %00000010, %00000000	; ch B
	db	%00100100, %00100000, %00000100, %00000000	; ch C

;----------
;音量設定
__fme7_volume_set:
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
	and	#%00011111		;bit 4はハードエンベロープフラグ
	sta	fme7_volume,x
	tay
	jsr	fme7_volume_write_sub

	jsr	sound_data_address
	jmp	__fme7_next_cmd

	;ソフトエンベ有効化
.softenve_part:
	jsr	volume_sub
	jmp	__fme7_next_cmd

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
	ldy	#$00			;ボリューム0を書き込み
	jsr	fme7_volume_write_sub

	jsr	sound_data_address
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ピッチLFO設定
.lfo_set:
	cmp	#CMD_SOFTLFO
	bne	.detune_set
	jsr	lfo_set_sub
	jmp	__fme7_next_cmd

;----------
;デチューン設定
.detune_set:
	cmp	#CMD_DETUNE
	bne	.pitch_set
	jsr	detune_sub
	jmp	__fme7_next_cmd

;----------
;ピッチエンベロープ設定
.pitch_set:
	cmp	#CMD_PITCHENV
	bne	.arpeggio_set
	jsr	pitch_set_sub
	jmp	__fme7_next_cmd

;----------
;ノートエンベロープ設定
.arpeggio_set:
	cmp	#CMD_NOTEENV
	bne	.freq_direct_set
	jsr	arpeggio_set_sub
	jmp	__fme7_next_cmd

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
	jmp	__fme7_next_cmd

;----------
;ウェイト設定
.wait_set:
	cmp	#CMD_WAIT
	bne	.hwenv_speed_set
	jsr	wait_sub
	rts				;音長を伴うコマンドなのでこのまま終了

;----------
;ハードウェアエンベロープ速度設定
.hwenv_speed_set:
	cmp	#CMD_FME7_HWENV_SPEED
	bne	.noise_set
	jsr	sound_data_address
	ldy	#$0B
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	iny
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	jsr	sound_data_address
	jmp	__fme7_next_cmd

;----------
;ノイズ周波数設定
.noise_set:
	cmp	#CMD_FME7_NOISE_FREQ
	bne	.keyon_set
	jsr	sound_data_address
	ldy	#$06
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	jsr	sound_data_address
	jmp	__fme7_next_cmd

;----------
;キーオンコマンド
.keyon_set:
	;XXX:知らないコマンドが来たときの処理はあったほうが良いかも
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	fme7_freq_set		;周波数セットへ
	jsr	effect_init
	rts				;音長を伴うコマンドなのでこのまま終了


;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------

;--------------------
; fme7_write_volume_and_freq : 音量レジスタと分周器レジスタへ書き込む
;
; 入力:
;	x : channel_selx2
;	fme7_volume,x : ボリューム値
;	sound_freq_{low,high,n106},x : 分周器レジスタの値
; 副作用:
;	a : 破壊
;	y : 破壊
;	音源 : 反映
;
fme7_write_volume_and_freq:
	ldy	fme7_volume,x
	jsr	fme7_volume_write_sub
	;fallthrough

;--------------------
; sound_fme7_write : 分周器レジスタへ書き込む
;
; 入力:
;	x : channel_selx2
;	fme7_volume,x : ボリューム値
;	sound_freq_{low,high,n106},x : 分周器レジスタの値
; 副作用:
;	a : 破壊
;	y : 破壊
;	音源 : 反映
; 備考:
;	XXX:サブルーチン名
;
sound_fme7_write:
	lda	fme7_tone,x
	cmp	#$02
	beq	.noise_freq_write	;ノイズ時はノイズ周波数を出力

	;トーン周波数設定
	ldy	fme7_ch_selx2		;周波数レジスタ番号
	lda	sound_freq_low,x
	sty	FME7_ADDR
	sta	FME7_DATA
	iny
	lda	sound_freq_high,x
	sty	FME7_ADDR
	sta	FME7_DATA
	rts

	;ノイズ周波数番号
.noise_freq_write:
	ldy	#$06
	lda	sound_freq_low,x
	and	#%00011111
	sty	FME7_ADDR
	sta	FME7_DATA
	rts


;--------------------
; fme7_volume_write_sub : 音量レジスタ/ハードエンベ波形レジスタ書き込み
;
; 入力:
;	y : 音量/波形($0-$f:音量、$10-$1f:波形)
;
fme7_volume_write_sub:
	cpy	#$10
	bcc	.write_vol		;音量レジスタ設定へ($0-$f)

	;ハードエンベロープ波形指定($10-$1f)
	tya
	and	#%00001111		;下位4ビットを取り出す
	ldy	#$0D
	sty	FME7_ADDR
	sta	FME7_DATA
	;ハードウェアエンベロープイネーブルを音量レジスタに書き込む
	ldy	#$10

.write_vol:
	;音量レジスタの処理
	lda	fme7_vol_regno
	sta	FME7_ADDR
	sty	FME7_DATA
	rts


;-------------------------------------------------------------------------------
;各エフェクトのフレーム処理サブルーチン
;-------------------------------------------------------------------------------

;--------------------
;sound_fme7_softenve : ソフトウェアエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	音量 : 反映
;	(以下volume_enve_subからの間接的な副作用)
;	soft_add_{low,high},x : 反映
;	バンク : softenve_tableのあるバンク
; 備考:
;	XXX:サブルーチン名
;
sound_fme7_softenve:
	jsr	volume_enve_sub
	sta	fme7_volume,x
	tay
	jsr	fme7_volume_write_sub
	jmp	enverope_address


;--------------------
; sound_fme7_lfo : ピッチLFOのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
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
sound_fme7_lfo:
	jsr	lfo_sub
	jmp	sound_fme7_write


;--------------------
; sound_fme7_pitch_enve : ピッチエンベロープのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	音程 : 反映
;	(以下pitch_subからの間接的な副作用)
;	pitch_add_{low,high},x : 反映
;	sound_freq_{low,high},x : 反映
;	バンク : #bank(pitchenve_table)
; 備考:
;	XXX:サブルーチン名
;
sound_fme7_pitch_enve:
	jsr	pitch_sub
	jsr	sound_fme7_write
	jmp	pitch_enverope_address


;--------------------
; sound_fme7_tone_enve : 音色エンベロープの処理
;
; 備考:
;	実体はfme7_tone_enve_subなのでそちらを参照
;
sound_fme7_tone_enve:
	jmp	fme7_tone_enve_sub


;--------------------
; sound_fme7_note_enve : ノートエンベロープのフレーム処理
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
sound_fme7_note_enve
	jsr	note_enve_sub
	bcs	.done			;0なので書かなくてよし
	jsr	fme7_freq_set
	jsr	sound_fme7_write
.done:
	jmp	arpeggio_address


;--------------------
; sound_fme7_tone_enve : 音色エンベロープの処理
;
; 副作用:
;	音色 : 反映
;	バンク : dutyenve_tableのあるバンク
; 備考:
;	XXX:sound_duty_enveropeと共通化できそう
;
fme7_tone_enve_sub:
.retry:
	ldx	<channel_selx2

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	indirect_lda	duty_add_low		;エンベロープデータ読み込み
	cmp	#$ff				;末尾かどうか
	beq	.do_repeat			;末尾ならばリピート処理へ
	jsr	fme7_tone_set_sub		;音色を設定する
	jmp	duty_enverope_address		;アドレス一個増やして終了

.do_repeat:
	; リピートテーブルを参照してデータアドレスをリストアする
	lda	duty_sel,x
	asl	a
	tay

	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	.retry
