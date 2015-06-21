;-------------------------------------------------------------------------------
;internal 4 dpcm 1 fds 1 vrc7 6 vrc6 3 n106 8 fme7 3 mmc5 3(?)
;4+1+1+6+3+8+3+3=29ch
MAX_CH		=	29
CH_COUNT	=	PTR_TRACK_END
	.if	CH_COUNT > MAX_CH
	.fail	"memory out of range"
	.endif

;-------------------------------------------------------------------------------
;memory definition
;-------------------------------------------------------------------------------
;ゼロページメモリ定義
	.zp
	.org	$00

t0			.ds	1		; temp
t1			.ds	1		;
t2			.ds	1		;
t3			.ds	1		;

sound_add_low		.ds	1		;command address
sound_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2

channel_sel		.ds	1		;
channel_selx2		.ds	1		;
channel_selx4		.ds	1		;

drvtmp0			.ds	1		; 各音源のドライバ内で使用する
drvtmp1			.ds	1		;
drvtmp2			.ds	1		;
drvtmp3			.ds	1		;

ind_lda_add		.ds	2		; for indirect_lda

temp_data_add		.ds	2		;

VRC6_DST_REG_LOW	.ds	1		; for vrc6.h
VRC6_DST_REG_HIGH	.ds	1		;

t4			.ds	1 ; for divider
t5			.ds	1 ;
t6			.ds	1 ;
t7			.ds	1 ;

ps_temp			.ds	1 ; for PS command

	.ifdef USE_TOTAL_COUNT
total_count .ds 2
	.endif


;-----------------------------------
;非ゼロページのメモリ定義

	.bss
BSS_BASE	=	$0200
	.org	BSS_BASE

;各チャンネルに必要なメモリ
;	ldx	<channel_selx2
;	してから
;	lda	memory,x
;	するので1バイトおきにデータがならぶ

;_add_lowと_add_highは近接している必要がある

soft_add_low		.ds	1		;software envelope(@v) address
soft_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
pitch_add_low		.ds	1		;pitch envelope (EP) address
pitch_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
duty_add_low		.ds	1		;duty envelope (@@) address
duty_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
arpe_add_low		.ds	1		;note envelope (EN) address
arpe_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_reverse_counter	.ds	1		;vibrato (MP) parameters
lfo_adc_sbc_counter	.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_start_counter	.ds	1		;
lfo_start_time		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_adc_sbc_time	.ds	1		;
lfo_depth		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_reverse_time	.ds	1		;
lfo_sel			.ds	1		;vibrato (MP) no
			.ds	CH_COUNT*2 - 2
detune_dat		.ds	1		;detune value
fme7_tone
register_high		.ds	1		;$4001+ch*4(internal)
			.ds	CH_COUNT*2 - 2
fme7_volume
register_low		.ds	1		;$4000+ch*4(internal)
duty_sel		.ds	1		;duty envelope no
			.ds	CH_COUNT*2 - 2
channel_loop		.ds	1		;|: :| loop counter
rest_flag		.ds	1		;
			.ds	CH_COUNT*2 - 2
softenve_sel		.ds	1		;software envelope(@v) no
pitch_sel		.ds	1		;pitch envelope (EP) no
			.ds	CH_COUNT*2 - 2
arpeggio_sel		.ds	1		;note envelope (EN) no
effect_flag		.ds	1		;
			.ds	CH_COUNT*2 - 2
sound_sel		.ds	1		;note no.
sound_counter		.ds	1		;wait counter
			.ds	CH_COUNT*2 - 2
sound_freq_low		.ds	1		;
sound_freq_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
sound_freq_n106		.ds	1		;n106じゃないchでも使ってる
sound_bank		.ds	1		;
			.ds	CH_COUNT*2 - 2
pitch_shift_amount	.ds	1		;
n106_volume
vrc7_key_stat
			.ds	1		;
			.ds	CH_COUNT*2 - 2
n106_7c
vrc7_volume
			.ds	1		;
extra_mem2		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_step			.ds	1		;
ps_count		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_nextnote		.ds	1		;
ps_dummy		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_addfreq_l		.ds	1		;
ps_addfreq_h		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

effect2_flags		.ds	1
sound_lasthigh		.ds	1
			.ds	CH_COUNT*2 - 2	;

;-------------
;その他

temporary		.ds	1		;
temporary2		.ds	1		;

fds_hard_select		.ds	1		;
fds_volume		.ds	1		;

n106_7f			.ds	1		;

fds_hwlfo_delay_counter	.ds	1		;
fds_hwlfo_delay_time	.ds	1		;

initial_wait		.ds	1		;

fme7_ch_sel		.ds	1		;
fme7_ch_selx2		.ds	1		;
fme7_ch_selx4		.ds	1		;
fme7_reg7		.ds	1		;R7現在値
fme7_vol_regno		.ds	1		;

DMC_NMI:
ram_nmi			.ds	3		;
DMC_RESET:
ram_reset		.ds	3		;
DMC_IRQ:
ram_irq			.ds	3		;

;effect_flag               DLLLadpv
EFF_DETUNE_ENABLE	= %10000000 ; detune flag
EFF_SOFTLFO_VSPEED	= %01000000 ; software LFOスピード可変フラグ（予約）
EFF_SOFTLFO_DIR		= %00100000 ; software LFO方向フラグ0=- 1=+
EFF_SOFTLFO_ENABLE	= %00010000 ; software LFO flag
EFF_NOTEENV_ENABLE	= %00001000 ; note envelope flag
EFF_DUTYENV_ENABLE	= %00000100 ; (internal)duty envelope flag (shared)
EFF_FDS_HWLFO_ENABLE	= %00000100 ; (FDS)hardware effect flag (shared)
EFF_PITCHENV_ENABLE	= %00000010 ; pitch envelope flag
EFF_SOFTENV_ENABLE	= %00000001 ; software envelope flag
EFF_SOFTLFO_MASK	= EFF_SOFTLFO_VSPEED | EFF_SOFTLFO_DIR | EFF_SOFTLFO_ENABLE

;effect2_flags
EFF2_HWENV_MASK		= %00110000 ; hw env mask (bit position of this mask depends on $4000 register.  cannot move.)
EFF2_SMOOTH_ENABLE	= %00000010
EFF2_SLUR_ENABLE	= %00000001

;rest_flag
RESTF_REST		= %00000001 ; rest
RESTF_KEYON		= %00000010 ; key on (if set, must do sound_data_write)

;command no.
CMD_KEYON_BEGIN		= $00
CMD_KEYON_END		= $8f
CMD_LOOP1		= $a0
CMD_LOOP2		= $a1
CMD_FDS_MODFREQ		= $e6 ; fds
CMD_PITCH_SHIFT		= $e7
CMD_SMOOTH		= $e8
CMD_SLUR		= $e9
CMD_BANK_SWITCH		= $ee
CMD_PITCH_SHIFT_AMOUNT	= $ef ; n106
CMD_HWENV		= $f0
CMD_FME7_NOISE_FREQ	= $f1 ; fme6
CMD_FME7_HWENV_SPEED	= $f2 ; fme6
CMD_FDS_HWLFO		= $f3 ; fds
CMD_WAIT		= $f4
CMD_WRITE_REG		= $f5
CMD_DIRECT_FREQ		= $f6
CMD_NOTEENV		= $f7
CMD_PITCHENV		= $f8
CMD_SWEEP		= $f9
CMD_DETUNE		= $fa
CMD_SOFTLFO		= $fb
CMD_REST		= $fc
CMD_VOLUME		= $fd
CMD_TONE		= $fe
CMD_END			= $ff


	.code

;-------------------------------------------------------------------------------
;macros and misc sub routines
;-------------------------------------------------------------------------------

;--------------------
; indirect_lda : ゼロページではないアドレスnでlda [n,x]を実現する
;
; statement
;	indirect_lda	hoge_add_low	;hoge_add_low is not zero page address
; is same as:
;	lda	[hoge_add_low,x]
;
; 副作用:
;	x : channel_selx2が再ロードされる(冗長?)
;	ind_lda_add : 破壊
;
indirect_lda	.macro
	lda	\1,x		;hoge_add_low
	sta	<ind_lda_add
	lda	\1+1,x		;hoge_add_high
	sta	<ind_lda_add+1
	ldx	#$00
	lda	[ind_lda_add,x]
	ldx	<channel_selx2	;XXX: 冗長?
	.endm

;--------------------
; channel_sel_inc : チャンネル番号を一つ増やす
;
; 入力:
;	channel_sel : チャンネル番号
; 出力:
;	channel_sel : 1増える
;	channel_selx2 : channel_sel*2
;	channel_selx4 : channel_sel*4
; 副作用:
;	a : 破壊
;
channel_sel_inc:
	inc	<channel_sel
	lda	<channel_sel
	asl	a
	sta	<channel_selx2
	asl	a
	sta	<channel_selx4
	rts

;-------------------------------------------------------------------------------
;initialize routine
;-------------------------------------------------------------------------------
INITIAL_WAIT_FRM = $00 ;最初にこのフレーム数だけウェイト

;--------------------
; sound_init : 初期化ルーチン
;
; 入力:
;	a : 現在の曲番号(複数ある場合)
;
sound_init:
	.if TOTAL_SONGS > 1
		;曲が複数ある場合、曲番号がAレジスタに入っているので
		;スタックに保存する
		pha
	.endif

	;ワークエリアのクリア
	lda	#$00
	ldx	#$00
.memclear_loop:
	sta	$0000,x
	sta	$0200,x
	sta	$0300,x
	sta	$0400,x
	sta	$0500,x
	sta	$0600,x
	sta	$0700,x
	inx
	bne	.memclear_loop

	;初期ウェイト
	lda	#INITIAL_WAIT_FRM
	sta	initial_wait

	;内蔵音源初期化
	;XXX:他の音源と同じようにinternal.hに移動すべきでは
	lda	#$0f
	sta	$4015		;チャンネル使用フラグ
	lda	#$08
	sta	$4001		;矩形波o2a以下対策
	sta	$4005

	.if (DPCM_BANKSWITCH)
		;DPCMのバンクスイッチを許可している場合、
		;各ベクタのthunkをRAM上に作る
		ldx	#$4C		; X = "JMP Absolute"
		stx	ram_nmi
		lda	$FFFA		; NMI low
		sta	ram_nmi+1
		lda	$FFFB		; NMI high
		sta	ram_nmi+2

		stx	ram_reset
		lda	$FFFC		; RESET low
		sta	ram_reset+1
		lda	$FFFD		; RESET high
		sta	ram_reset+2

		stx	ram_irq
		lda	$FFFE		; IRQ/BRK low
		sta	ram_irq+1
		lda	$FFFF		; IRQ/BRK high
		sta	ram_irq+2
	.endif ; DPCM_BANKSWITCH


	.if	SOUND_GENERATOR & __FME7
	jsr	fme7_sound_init
	.endif

	.if	SOUND_GENERATOR & __MMC5
	jsr	mmc5_sound_init
	.endif

	.if	SOUND_GENERATOR & __FDS
	jsr	fds_sound_init
	.endif

	.if	SOUND_GENERATOR & __N106
	jsr	n106_sound_init
	.endif

	.if	SOUND_GENERATOR & __VRC6
	jsr	vrc6_sound_init
	.endif

	.if TOTAL_SONGS > 1
		;曲が複数ある場合、曲番号に対応するテーブルの位置を求めておく
; use t0, t1, t2, t3
.start_add_lsb	=	t0
.start_add_lsb_hi=	t1
.start_bank	=	t2
.start_bank_hi	=	t3

		pla		; A=曲番号
		asl	a
		tax

		lda	song_addr_table,x
		sta	<.start_add_lsb
		lda	song_addr_table+1,x
		sta	<.start_add_lsb_hi

		.if (ALLOW_BANK_SWITCH)
			lda	song_bank_table,x
			sta	<.start_bank
			lda	song_bank_table+1,x
			sta	<.start_bank_hi
		.endif

	.endif

	;チャンネル別の(音源非依存で基本的な)ワークエリアの初期化
.do_channel_init:
	lda	#$00
	sta	<channel_sel
	sta	<channel_selx2
	sta	<channel_selx4
.channel_init_loop:
	lda	<channel_sel
	cmp	#PTR_TRACK_END		;終わり？
	beq	.done

	.if TOTAL_SONGS > 1
		;曲が複数ある
		.if (ALLOW_BANK_SWITCH)
			ldy	<channel_sel		; y = ch; x = ch<<1;
			ldx	<channel_selx2

			lda	[.start_bank],y
			sta	sound_bank,x

			ldy	<channel_selx2		; x = y = ch<<1;
		.else
			ldx	<channel_selx2		; x = y = ch<<1;
			ldy	<channel_selx2
		.endif
		lda	[.start_add_lsb],y
		sta	<sound_add_low,x	;データ開始位置書き込み
		iny
		lda	[.start_add_lsb],y
		sta	<sound_add_high,x	;データ開始位置書き込み
	.else
		;曲が1つだけ
		ldy	<channel_sel		; y = ch; x = ch<<1;
		ldx	<channel_selx2
		.if (ALLOW_BANK_SWITCH)
			lda	song_000_bank_table,y
			sta	sound_bank,x
		.endif
		lda	song_000_track_table,x
		sta	<sound_add_low,x	;データ開始位置書き込み
		lda	song_000_track_table+1,x
		sta	<sound_add_low+1,x	;データ開始位置書き込み

	.endif

	; x = channel_selx2
	lda	#$ff
	sta	sound_lasthigh,x
	lda	#$00
	sta	effect_flag,x
	lda	#$01
	sta	sound_counter,x

	jsr	channel_sel_inc

	jmp	.channel_init_loop

.done:
	rts


;-------------------------------------------------------------------------------
;main routine
;-------------------------------------------------------------------------------

;--------------------
; sound_driver_start : フレーム処理のエントリポイント
;
sound_driver_start:
	.ifdef USE_TOTAL_COUNT
		;フレームカウンタ
		inc	<total_count
		bne	.skip_tcount
		inc	<total_count+1
.skip_tcount:
	.endif ; USE_TOTAL_COUNT

	;初期ウェイト
	lda	initial_wait
	beq	.initial_wait_done
	dec	initial_wait
	rts
.initial_wait_done:

	;チャンネル番号0からカウントアップしてゆく
	lda	#$00
	sta	<channel_sel
	sta	<channel_selx2
	sta	<channel_selx4

	; 各音源ごとの再生ルーチン
	; 内蔵音源ループ
.internal_loop:
	jsr	sound_internal
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#$04
	bne	.internal_loop

	; DPCM
;	.if	DPCMON
		jsr	sound_dpcm
;	.endif
	jsr	channel_sel_inc

	; FDS
	.if	SOUND_GENERATOR & __FDS
		jsr	sound_fds
		jsr	channel_sel_inc
	.endif

	; VRC7
	.if	SOUND_GENERATOR & __VRC7
.vrc7_loop:
		jsr	sound_vrc7
		jsr	channel_sel_inc
		lda	<channel_sel
		cmp	#PTRVRC7+$06
		bne	.vrc7_loop
	.endif

	; VRC6
	.if	SOUND_GENERATOR & __VRC6
.vrc6_loop:
		jsr	sound_vrc6
		jsr	channel_sel_inc
		lda	<channel_sel
		cmp	#PTRVRC6+$03
		bne	.vrc6_loop
	.endif

	; N106
	.if	SOUND_GENERATOR & __N106
.n106_loop:
		jsr	sound_n106

		; 定義バンク切り替え
		lda	#bank(n106_channel)*2
		jsr	change_bank

		jsr	channel_sel_inc
		; N106チャンネル終了判定
		lda	<channel_sel
		sec
		sbc	#PTRN106
		cmp	n106_channel ; チャンネル数 XXX:何故equで定義しないのか
		bne	.n106_loop
	.endif

	; FME7
	.if	SOUND_GENERATOR & __FME7
.fme7_loop:
		jsr	sound_fme7
		jsr	channel_sel_inc
		lda	<channel_sel
		cmp	#PTRFME7+$03
		bne	.fme7_loop
	.endif

	; MMC5
	.if	SOUND_GENERATOR & __MMC5
.mmc5_loop:
		jsr	sound_mmc5
		jsr	channel_sel_inc
		lda	<channel_sel
		cmp	#PTRMMC5+$02
		bne	.mmc5_loop
	.endif

	rts


;------------------------------------------------------------------------------
;command read sub routines
;------------------------------------------------------------------------------

;--------------------
; sound_data_address : サウンドデータのアドレスに１を足す
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 足される前の値
; 出力:
;	sound_add_{low,high},x : 16ビット値として1足される
; 備考:
;	XXX:サブルーチン名
;
sound_data_address:
	inc	<sound_add_low,x	;データアドレス(lo)+1
	bne	__sound_data_address_done
					;位が上がったら
_sound_data_address_inc_high:		;sound_data_address_add_aから飛んでくる
	inc	<sound_add_high,x	;データアドレス(hi)+1
__sound_data_address_done:
	rts


;--------------------
; sound_data_address_add_a : サウンドデータのアドレスにaを足す
;
; 入力:
;	a : 加算値
;	x : channel_selx2
;	sound_add_{low,high},x : 足される前の値
; 出力:
;	sound_add_{low,high},x : 16ビット値としてaが足される
; 副作用:
;	a : 破壊される
; 備考:
;	bcsするので上の_sound_data_address_inc_highの近くにないといけない
;
sound_data_address_add_a:
	clc
	adc	<sound_add_low,x
	sta	<sound_add_low,x
	bcs	_sound_data_address_inc_high
	rts


;--------------------
; change_bank : バンク切り替え
;
; 入力:
;	a : バンク番号
; 副作用:
;	現在対応しているマッパはNSFのみ
;	  → 対応マッパが増えると下記ウィンドウのアドレスが変わる可能性がある
;	A000h-AFFFh : バンク a   に切り替わる if a   <= BANK_MAX_IN_4KB
;	B000h-BFFFh : バンク a+1 に切り替わる if a+1 <= BANK_MAX_IN_4KB
;
change_bank:
	if (ALLOW_BANK_SWITCH)
		;バンク切り替えできるcondition: A <= BANK_MAX_IN_4KB
		;i.e. A < BANK_MAX_IN_4KB + 1
		;i.e. A - (BANK_MAX_IN_4KB+1) < 0
		;i.e. NOT ( A - (BANK_MAX_IN_4KB+1) >= 0 )
		;skipするcondition: A - (BANK_MAX_IN_4KB+1) >= 0
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5ffa ; A000h-AFFFh
		clc
		adc	#$01
		cmp	#BANK_MAX_IN_4KB+1
		bcs	.avoidbankswitch
		sta	$5ffb ; B000h-BFFFh
.avoidbankswitch
	endif
	rts


;-------------------------------------------------------------------------------
; loop_sub : 音源チップ非依存なリピートコマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <count> <bank> <addr_low> <addr_high>
;		↑ここを指す
; 出力:
;	sound_bank,x / sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	channel_loop,x : 反映
; 備考:
;	すぐ下にあるloop_sub2も参照のこと
;
; channel_loop[x]++;
; if (channel_loop[x] == <num>) {
;   channel_loop[x] = 0;
;   残りのパラメータ無視してadrを次に進める;
; } else {
;   0xeeコマンドと同じ処理;
; }
;
loop_sub:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;繰り返し回数
	beq	.loop_done
	; 無条件ジャンプ処理(0xeeコマンドと同等)
	jsr	sound_data_address	;次の位置に進める
	jmp	bank_address_change	;バンク/アドレスL/アドレスHを読んで設定
.loop_done:
	; 明示的にループ回数をリセットするようなループ始点コマンドは無いので、
	; 次の機会のためにループ回数をクリアしておく必要がある
	lda	#$00
	sta	channel_loop,x
_loop_esc_through:			;loop_sub2から飛んでくる
	lda	#$04			;回数/バンク/アドレスL/Hをスキップ
	jsr	sound_data_address_add_a
	rts


;-----------
; loop_sub2 : 音源チップ非依存なリピート中断コマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <count> <bank> <addr_low> <addr_high>
;		↑ここを指す
; 出力:
;	sound_bank,x / sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	channel_loop,x : 反映
; 備考:
;	loop_subとはリピート継続時と終了時の処理が逆
;	+-----------+--------------+--------------+
;	|           | ループ継続時 | ループ終了時 |
;	+-----------+--------------+--------------+
;	| loop_sub  | (後方へ)jump | fallthrough  |
;	| loop_sub2 | fallthrough  | (前方へ)jump |
;	+-----------+--------------+--------------+
;
;	bneするので、Loop_subの近くにある必要がある
;
;	XXX:サブルーチン名
;
; channel_loop++;
; if (channel_loop == <num>) {
;   channel_loop = 0;
;   0xeeコマンドと同じ処理;
; } else {
;   残りのパラメータ無視してadrを次に進める;
; }
;
loop_sub2:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;繰り返し回数
	bne	_loop_esc_through
	lda	#$00
	sta	channel_loop,x
	jsr	sound_data_address
	jmp	bank_address_change


;--------------------
; data_bank_addr : 音源チップ非依存なバンクセットコマンドの処理
; bank_address_change : 別のコマンドから利用されるジャンプ処理サブルーチン
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		(data_bank_addrの場合)
;			<cmd> <bank> <addr_low> <addr_high>
;			↑ここを指す
;		(bank_address_changeの場合)
;			<cmd> ... <bank> <addr_low> <addr_high>
;				  ↑ここを指す
; 出力:
;	sound_bank,x / sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
; 備考:
;	バンク切り替え自体は行わない
;
data_bank_addr:
	jsr	sound_data_address
bank_address_change:
	if (ALLOW_BANK_SWITCH)
		lda	[sound_add_low,x]
		sta	sound_bank,x
	endif

	jsr	sound_data_address
	lda	[sound_add_low,x]
	pha
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<sound_add_high,x
	pla
	sta	<sound_add_low,x	;新しいアドレス書込み

	rts


;--------------------
;data_end_sub:
;	ldy	<channel_sel
;
;	if (ALLOW_BANK_SWITCH)
;	lda	loop_point_bank,y
;	sta	sound_bank,x
;	endif
;
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;ループ開始位置書き込み Low
;	inx
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;ループ開始位置書き込み High
;	rts


;--------------------
; volume_sub : 音源チップ非依存なソフトエンベロープ有効化コマンドの処理
;
; 入力:
;	x : channel_selx2
;	temporary : ソフトエンベロープ番号
;	sound_add_{low,high},x : 現在のサウンドデータアドレス(XXX)
;		<cmd> <number_of_env>
;		      ↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	y : 破壊
;	バンク : softenv_tableのあるバンク
;	effect_flag,x : ソフトエンベロープフラグが立てられる
;	softenve_sel,x : ソフトエンベロープ番号になる
;	soft_add_{low,high},x : 所定のアドレスで初期化される
; 備考:
;	XXX:サブルーチン名なんとかならないか
;	XXX:サブルーチン化が他のコマンドと比べて中途半端
;
volume_sub:
	lda	effect_flag,x
	ora	#EFF_SOFTENV_ENABLE
	sta	effect_flag,x		;ソフトエンベ有効指定

	lda	temporary
	sta	softenve_sel,x
	asl	a
	tay				;y = ソフトエンベロープ番号*2

	lda	#bank(softenve_table)*2
	jsr	change_bank

	lda	softenve_table,y	;ソフトエンベデータアドレス設定
	sta	soft_add_low,x
	lda	softenve_table+1,y
	sta	soft_add_high,x
	jsr	sound_data_address
	rts


;--------------------
; lfo_set_sub : 音源チップ非依存なピッチLFO有効化コマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <number_of_lfo>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	y : 破壊
;	t0, t1 : 破壊
;	effect_flag,x : LFO関連フラグが操作される
;	(以下はLFO有効時のみ - LFO番号 != ffh)
;	lfo_sel,x : LFO番号*4
;	バンク : lfo_dataのあるバンク
;	lfo_start_time,x
;	lfo_start_counter,x
;	lfo_reverse_time,x
;	lfo_reverse_counter,x
;	lfo_depth,x
;	lfo_adc_sbc_time,x
;	lfo_adc_sbc_counter,x : 適宜設定される
; 備考:
;	XXX:サブルーチン名なんとかならないか
;
lfo_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	.lfo_enable

	; 無効化
	lda	effect_flag,x
	and	#~EFF_SOFTLFO_MASK
	sta	effect_flag,x
	jsr	sound_data_address
	rts

	; 有効化
.lfo_enable:
	asl	a
	asl	a
	sta	lfo_sel,x

	tay	; y = LFO番号*4

	; 定義テーブルにバンクを切り替え
	lda	#bank(lfo_data)*2
	jsr	change_bank

	ldx	<channel_selx2	; XXX:change_bankでxは破壊されないので不要
	lda	lfo_data,y
	sta	lfo_start_time,x		;ディレイセット
	sta	lfo_start_counter,x
	lda	lfo_data+1,y
	sta	lfo_reverse_time,x		;スピードセット
	sta	lfo_reverse_counter,x
	lda	lfo_data+2,y
	sta	lfo_depth,x			;仮デプスセット(後でwarizan_startにより書き換わる)
;	lda	lfo_data+3,y
;	sta	lfo_harf_time,x
;	sta	lfo_harf_count,x		;1/2カウンタセット

	jsr	warizan_start

	.if PITCH_CORRECTION
		;変化方向を表引きして訂正する
		lda	effect_flag,x
		ora	#EFF_SOFTLFO_ENABLE	;LFO有効フラグセット
		sta	effect_flag,x
		jsr	lfo_initial_vector
	.else
		;内蔵音源かどうかでしか判定しない簡易ルーチン
		lda	<channel_sel
		sec
		sbc	#$05
		bcc	.for_external_chip

		;内蔵音源
		lda	effect_flag,x
		ora	#EFF_SOFTLFO_DIR | EFF_SOFTLFO_ENABLE
		sta	effect_flag,x
		jmp	.dir_done

		;拡張音源では値の変化方向を逆にする
.for_external_chip
		lda	effect_flag,x
		and	#~EFF_SOFTLFO_DIR	;波形−処理
		ora	#EFF_SOFTLFO_ENABLE	;LFO有効フラグセット
		sta	effect_flag,x
.dir_done:
	.endif	; PITCH_CORRECTION

	jsr	sound_data_address
	rts


	.if PITCH_CORRECTION
;--------------------
; 音源チップの違いによるピッチ変化の方向を補正するサブルーチン
;
; 入力:
;	x : channel_selx2
; 副作用:
;	effect_flag,x : EFF_SOFTLFO_DIRビットが「音程上昇方向」に設定される
; 備考:
;	#PITCH-CORRECTIONモードでのみ有効。
;
lfo_initial_vector:
	lda	freq_vector_table,x
	bmi	.increasing_function
; 2A03など
.decreasing_function:
	lda	effect_flag,x
	and	#~EFF_SOFTLFO_DIR	;LFOは最初減算
	jmp	.done
; FDSなど
.increasing_function:
	lda	effect_flag,x
	ora	#EFF_SOFTLFO_DIR	;LFOは最初加算
.done:
	sta	effect_flag,x
	rts
	.endif	; PITCH_CORRECTION


;--------------------
; detune_sub : 音源チップ非依存なデチューンコマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <detune_factor>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	y : 破壊 XXX:何故か分からない
;	effect_flag,x : デチューンフラグが操作される
;	(以下detune_factor != ffhのとき)
;	detune_dat,x : デチューンファクター
; 備考:
;	XXX: サブルーチン名
;
detune_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	.detune_enable

	; 無効化
	lda	effect_flag,x
	and	#~EFF_DETUNE_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts

	; 有効化
.detune_enable:
	tay				; XXX: 不要(呼び出し元も参照してない)
	sta	detune_dat,x
	lda	effect_flag,x
	ora	#EFF_DETUNE_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts


;--------------------
; pitch_set_sub : 音源チップ非依存なピッチエンベロープコマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <penv_number>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	y : 破壊
;	effect_flag,x : ピッチエンベロープフラグが操作される
;	(以下penv_number != ffhのとき)
;	pitch_sel,x : ピッチエンベロープ番号
;	バンク : pitchenve_tableのあるバンク
;	pitch_add_{low,high},x : ピッチエンベロープデータアドレス
; 備考:
;	XXX: サブルーチン名
;
pitch_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	.pitch_envelope_enable

	; 無効化
	lda	effect_flag,x
	and	#~EFF_PITCHENV_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts

	; 有効化
.pitch_envelope_enable:
	sta	pitch_sel,x
	asl	a
	tay

	; 定義バンク切り替え
	lda	#bank(pitchenve_table)*2
	jsr	change_bank

	lda	pitchenve_table,y
	sta	pitch_add_low,x
	lda	pitchenve_table+1,y
	sta	pitch_add_high,x

	lda	effect_flag,x
	ora	#EFF_PITCHENV_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts


;--------------------
; arpeggio_set_sub : 音源チップ非依存なノートエンベロープコマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <nenv_number>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	y : 破壊
;	effect_flag,x : ノートエンベロープフラグが操作される
;	(以下nenv_number != ffhのとき)
;	arpeggio_sel,x : ノートエンベロープ番号
;	バンク : arpeggio_tableのあるバンク
;	arpe_add_{low,high},x : ピッチエンベロープデータアドレス
; 備考:
;	XXX: サブルーチン名
;
arpeggio_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	.arpeggio_enable

	; 無効化
	lda	effect_flag,x
	and	#~EFF_NOTEENV_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts

	; 有効化
.arpeggio_enable:
	sta	arpeggio_sel,x
	asl	a
	tay

	; 定義バンク切り替え
	lda	#bank(arpeggio_table)*2
	jsr	change_bank

	lda	arpeggio_table,y
	sta	arpe_add_low,x
	lda	arpeggio_table+1,y
	sta	arpe_add_high,x

	lda	effect_flag,x
	ora	#EFF_NOTEENV_ENABLE
	sta	effect_flag,x
	jsr	sound_data_address
	rts


;--------------------
; direct_freq_sub : 音源チップ非依存なピッチ直接指定コマンドの処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <pitch_low> <pitch_high> <counter>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	effect_init呼び出しによる副作用が生じる
; 備考:
;	XXX:サブルーチン名
;	XXX:N106では最上位バイトが足りない
;
direct_freq_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_low,x		;Low
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_high,x		;High
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x			;Counter
	jsr	sound_data_address
	jsr	effect_init			;反映させる
	rts


;--------------------
; y_sub : 音源チップ非依存なメモリ書き込みコマンド(y)の処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <addr_low> <addr_high> <byte>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
; 備考:
;	XXX: サブルーチン名
;
y_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<t0
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<t1
	jsr	sound_data_address
	lda	[sound_add_low,x]
	ldx	#$00
	sta	[t0,x]
	ldx	<channel_selx2
	jsr	sound_data_address
	rts


;--------------------
; wait_sub : 音源チップ非依存な待機コマンド(w)の処理
;
; 入力:
;	x : channel_selx2
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <count>
;		↑ここを指す
; 出力:
;	sound_add_{low,high},x : 次のコマンドを指す
; 副作用:
;	a : 破壊
;	sound_counter,x : 待機カウント
; 備考:
;	XXX: サブルーチン名
;
wait_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x
	jsr	sound_data_address
	rts


;-------------------------------------------------------------------------------
;effect sub routines
;-------------------------------------------------------------------------------

;--------------------
; detune_write_sub : デチューンコマンドの値を周波数レジスタのワーク値に反映する
;
; 入力:
;	x : channel_selx2
;	detune_dat,x : 符号付きデチューン値
;	sound_freq_{low,high,n106},x : 加算前の周波数値
; 出力:
;	sound_freq_{low,high,n106},x : 加算後の周波数値
; 副作用:
;	a : 破壊
;	y : 破壊
; 備考:
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;
detune_write_sub:
	lda	effect_flag,x
	and	#EFF_DETUNE_ENABLE
	bne	.detune_is_enabled
	; デチューンが無効なら何もしない
	rts

	; デチューン有効
.detune_is_enabled:
	lda	detune_dat,x
	; fallthrough

;--------------------
; freq_add_mcknumber : 周波数レジスタの値に符号付き値を加算する
;
; 入力:
;	x : channel_selx2
;	a : 符号付き加算値(補数表現ではなく、符号1bit+仮数7bit形式)
;	sound_freq_{low,high,n106},x : 加算前の周波数値
; 出力:
;	sound_freq_{low,high,n106},x : 加算後の周波数値
; 副作用:
;	a : 破壊
;	y : 破壊
; 備考:
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;
freq_add_mcknumber:
	.if PITCH_CORRECTION
		;符号をテーブルで補正する
		eor	freq_vector_table,x
	.else
		;符号を補正しない
		eor	#0		;set N flag
	.endif
	bmi	__detune_minus
	; fallthrough

;--------------------
; detune_plus : 周波数値に値を加算する
;
; 入力:
;	x : channel_selx2
;	a : 加算値
;	sound_freq_{low,high,n106},x : 加算前の周波数値
; 出力:
;	sound_freq_{low,high,n106},x : 加算後の周波数値
; 副作用:
;	a : 破壊
;	y : 破壊
; 備考:
;	反映されるのはワークエリアの値だけで、実際にレジスタに書き込むのは
;	呼び出し元の仕事。
;
detune_plus:
	eor	#0			;set Z flag
	beq	.endo			;プラス0なら終了

	ldy	pitch_shift_amount,x
	bne	_detune_plus_with_asl	;シフトあり

	; シフトなし単純加算
	clc
	adc	sound_freq_low,x
	sta	sound_freq_low,x
	bcs	.carry		; XXX: .endoを最後に移動してbcc .endoにすべし
.endo:	rts
.carry:
	; 桁上がり
	inc	sound_freq_high,x
	bne	.no_extra_carry
	; さらに桁上がり
	inc	sound_freq_n106,x
.no_extra_carry:
	rts

__detune_minus:
	and	#%01111111
	; fallthrough

;--------------------
; detune_minus_nomask : 周波数値から値を減算する
;
; 入力:
;	x : channel_selx2
;	a : 減算値(補数表現ではなく、符号1bit+仮数7bit形式)
;	sound_freq_{low,high,n106},x : 減算前の周波数値
; 出力:
;	sound_freq_{low,high,n106},x : 減算後の周波数値
; 副作用:
;	a : 破壊
;	y : 破壊
;	t0 : 破壊
;	以下はpitch_shift_amount,xが0でないとき
;	t1 : 破壊
;	t2 : 破壊
; 備考:
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;
detune_minus_nomask:
	eor	#0			;set Z flag iff A==0
	beq	.endo			;プラス0なら終了

	ldy	pitch_shift_amount,x
	bne	_detune_minus_nomask_with_asl	;シフトあり

	sta	<t0
	lda	sound_freq_low,x
	sec
	sbc	<t0
	sta	sound_freq_low,x
	bcc	.borrow		; XXX: .endoを最後に移動してbcs .endoにすべし
.endo:	rts
.borrow:
	; 桁下がり
	lda	sound_freq_high,x
	beq	.extra_borrow
	dec	sound_freq_high,x
	rts
.extra_borrow:
	; さらに桁下がり
	dec	sound_freq_high,x
	dec	sound_freq_n106,x
	rts

	; _detune_plus_with_asl / _detune_minus_nomask_with_asl
	; 変化量に対する任意回の左シフトを伴うバージョン
	; 内部ルーチンなので直接呼び出さず、
	; freq_add_mcknumber, detune_plus, detune_minus_nomaskを経由すること
	; A = 足し算引き算する値
	; Y = シフト量 (0は禁止)
_detune_plus_with_asl:
	sta	<t0
	lda	#0
	sta	<t1
	sta	<t2
	beq	__do_detune_plus_with_asl	;常にZ=0なので、
						;無条件ジャンプと等価
						;(命令長が1バイト短く済む)

_detune_minus_nomask_with_asl:
	; A(1～127)を2の補数表現で符号反転(-1:FFh～-127:81h)してt0へストア
	eor	#$ff
	sta	<t0
	inc	<t0
	beq	__detune_through		;A=0なら何もしない
	; t0をt2/t1/t0からなる24bit符号付き整数へと符号拡張
	lda	#$ff	;t0は常に負数なので、t1/t2はFFhとなる
	sta	<t1
	sta	<t2

__do_detune_plus_with_asl:
	; t2/t1/t0 24bit値をY回左シフト
.loop
		asl	<t0
		rol	<t1
		rol	<t2
		dey
		bne	.loop

	; t2/t1/t0をsound_freq_{n106,high,low}に加算
	clc
	lda	<t0
	adc	sound_freq_low,x
	sta	sound_freq_low,x
	lda	<t1
	adc	sound_freq_high,x
	sta	sound_freq_high,x
	lda	<t2
	adc	sound_freq_n106,x
	sta	sound_freq_n106,x
__detune_through:
	rts


;--------------------
; sound_software_enverope : (内蔵音源専用)ソフトウェアエンベロープのフレーム処理
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
;	XXX:internal.hへ移動すべきでは
;	XXX:つづり
;
sound_software_enverope:
	jsr	volume_enve_sub
	sta	register_low,x
	ora	register_high,x		;音色データ（上位4bit）と音量（下位4bit）を合成
	ldy	<channel_selx4
	sta	$4000,y			;書き込み
	jsr	enverope_address	;アドレス一個増やして
	rts				;おしまい


;--------------------
; volume_enve_sub : ソフトウェアエンベロープのフレーム処理における音源非依存な音量値計算
;
; 出力:
;	a : 音量値
; 副作用:
;	x : channel_selx2
;	y : 破壊
;	バンク : softenve_tableのあるバンク
;	soft_add_{low,high},x : リピートマークを指していた場合には先頭に戻る
; 備考:
;	データを読み出した後、アドレス(soft_add_{low,high},x)を進めないので、
;	呼び出し元で進める必要がある。
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;
volume_enve_sub:
	lda	#bank(softenve_table)*2
	jsr	change_bank

	ldx	<channel_selx2
	indirect_lda	soft_add_low	;エンベロープデータ読み込み
	cmp	#$ff			;エンベロープデータの末尾かどうか
	beq	.do_repeat		;末尾ならリピート処理へ
	rts

.do_repeat:
	; リピートテーブルを参照してデータアドレスをリストアする
	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_lp_table,y
	sta	soft_add_low,x
	lda	softenve_lp_table+1,y
	sta	soft_add_high,x
	jmp	volume_enve_sub


;--------------------
; enverope_address : ソフトウェアエンベロープアドレスを1を足す
;
; 入力:
;	x : channel_selx2
;	soft_add_{low,high},x : 足される前の値
; 出力:
;	soft_add_{low,high},x : 16bit値として1足される
; 備考:
;	XXX:つづり
;
enverope_address:
	inc	soft_add_low,x
	bne	.done
	inc	soft_add_high,x
.done
	rts


;--------------------
; sound_duty_enverope : (内蔵音源専用)デューティー比エンベロープのフレーム処理
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
;	XXX:つづり
;	XXX:レジスタ書き込み部分を分離すればfme7_tone_enve_subと共通化できそう
;
sound_duty_enverope:
	ldx	<channel_selx2

	lda	<channel_sel
	cmp	#$02
	beq	.done			;三角波なら飛ばし〜

	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	indirect_lda	duty_add_low	;エンベロープデータ読み込み
	cmp	#$ff			;末尾かどーか
	beq	.do_repeat		;末尾ならリピート処理へ

	; ハードウェアエンベロープをregister_high,xへ反映
	pha
	lda	effect2_flags,x         ; hw_envelope
	and	#EFF2_HWENV_MASK
	eor	#EFF2_HWENV_MASK
	sta	register_high,x
	pla

	; デューティー比の値をregister_high,xの所定のビット位置へ反映
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
;	ora	#EFF2_HWENV_MASK	;hardware envelope & ... disable
	ora	register_high,x		;hw_envelope
	sta	register_high,x

	ora	register_low,x		;音色データ（上位4bit）と音量（下位4bit）を合成
	ldy	<channel_selx4
	sta	$4000,y			;書き込み
	jsr	duty_enverope_address	;アドレス一個増やす
.done:
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
	jmp	sound_duty_enverope


;--------------------
; duty_enverope_address : デューティー比エンベロープアドレスを1つ進める
;
; 入力:
;	x : channel_selx2
;	duty_add_{low,high},x : 足される前の値
; 出力:
;	duty_add_{low,high},x : 16bit値として1足される
; 備考:
;	XXX:つづり
;	XXX:internal.hへ移動すべきでは
;
duty_enverope_address:
	inc	duty_add_low,x
	bne	.done
	inc	duty_add_high,x
.done:
	rts


;--------------------
; sound_lfo : (内蔵音源専用)LFOのフレーム処理
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
;	XXX:internal.hへ移動すべきでは
;
sound_lfo:
	lda	sound_freq_high,x
	sta	temporary

	jsr	lfo_sub

	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y			;現在値をレジスタにセット
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	sta	$4003,y
.done:
	rts


;--------------------
; lfo_sub : LFOのフレーム処理における音源非依存な音量値計算
;
; 入力:
;	sound_freq_{low,high,n106},x : LFO処理前の音程
; 出力:
;	sound_freq_{low,high,n106},x : LFO処理の結果が反映される
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	y : 破壊
;	lfo_start_counter,x : 反映
;	lfo_reverse_counter,x : 反映
;	lfo_adc_sbc_counter,x : 反映
;	effect_flag,x : EFF_SOFTLFO_DIRビットが影響を受ける
; 備考:
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;	XXX:サブルーチン名
;
lfo_sub:
	ldx	<channel_selx2

	; LFOの開始を遅らせるフェーズ
	lda	lfo_start_counter,x
	beq	.do_lfo
	dec	lfo_start_counter,x
	rts

.do_lfo:
	; 変化方向を反転するか判定
	asl	lfo_reverse_time,x	;2倍する(一時的にLFOの1/2周期になる)
	lda	lfo_reverse_counter,x	;反転用カウンタ読み込み
	cmp	lfo_reverse_time,x	;LFOの周期の1/2ごとに反転する
	bne	.skip_reverse		;規定数に達していなければ反転をスキップ

		; 規定数に達していたら方向反転処理
		lda	#$00			;
		sta	lfo_reverse_counter,x	;反転カウンタ初期化
		lda	effect_flag,x		;方向ビットを反転
		eor	#EFF_SOFTLFO_DIR	;
		sta	effect_flag,x		;

.skip_reverse:
	lsr	lfo_reverse_time,x	;1/2にする(LFOの1/4周期に戻る)

	; 変化速度の処理
	lda	lfo_adc_sbc_counter,x	;変化速度カウンタ読み込み
	cmp	lfo_adc_sbc_time,x	;lfo_adc_sbc_timeごとに変分処理する
	bne	.delta_done		;まだなら変分処理をスキップする

		; 一致していれば変分処理
		lda	#$00			;
		sta	lfo_adc_sbc_counter,x	;変化速度カウンタ初期化
		lda	effect_flag,x		;＋か−か
		and	#EFF_SOFTLFO_DIR	;このビットが
		bne	.lfo_delta_plus		;立っていたら加算
			; 減算
			lda	lfo_depth,x
			jsr	detune_minus_nomask
			jmp	.delta_done
.lfo_delta_plus:
			; 加算
			lda	lfo_depth,x
			jsr	detune_plus

.delta_done:
	;カウンタ足してお終い
	inc	lfo_reverse_counter,x
	inc	lfo_adc_sbc_counter,x
	rts


;--------------------
; warizan_start : ピッチLFOのための割り算処理
;
; 入力:
;	x : channel_selx2
;	lfo_reverse_time,x : LFO周期の1/4
;	lfo_depth,x : LFOの振れ幅(Y軸ピーク)
; 出力:
;	lfo_depth,x : 一回あたりの変化量
;	lfo_adc_sbc_time,x : 変化させる時間間隔(フレーム数単位)
; 副作用:
;	a : 破壊
;	t0 : 破壊
;	t1 : 破壊
;	lfo_adc_sbc_counter,x : lfo_adc_sbc_time,xと同じ値で初期化される
; 備考:
;	XXX:サブルーチン名
;
warizan_start:
.quotient = t0
.divisor = t1
	lda	#$00
	sta	<.quotient
	lda	lfo_reverse_time,x	;1/4周期と
	cmp	lfo_depth,x		;Y軸ピーク指定
	beq	.plus_one		;同じなら1:1
	bmi	.depth_wari		;Y軸ピークのほうが大きい場合

.revers_wari:
	; 1/4周期のほうが大きい場合(=傾きが1未満のとき)には
	; (1/4周期)/(Y軸ピーク)フレームごとに1増減させる
	lda	lfo_depth,x
	sta	<.divisor
	lda	lfo_reverse_time,x
	jsr	.warizan
	lda	<.quotient
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	lda	#$01
	sta	lfo_depth,x
	rts

.depth_wari:
	; Y軸ピークのほうが大きい場合(=傾きが1超のとき)には
	; 1フレームごとに(Y軸ピーク)/(1/4周期)増減させる
	lda	lfo_reverse_time,x
	sta	<.divisor
	lda	lfo_depth,x
	jsr	.warizan
	lda	<.quotient
	sta	lfo_depth,x
	lda	#$01
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	rts

.plus_one:
	; 1/4周期とY軸ピークが等しい場合(=傾きがちょうど1のとき)には
	; 1フレームごとに1増減させる
	lda	#$01
	sta	lfo_depth,x
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	rts

	; 実際に割り算を行うサブルーチン
	; .quotient += floor(a/.divisor)
.warizan:
	inc	<.quotient
	sec
	sbc	<.divisor
	; XXX: 単にbcs .warizan じゃ駄目なの？
	beq	.warizan_end
	bcc	.warizan_end
	bcs	.warizan		;無条件ジャンプと等価
.warizan_end:
	rts


;--------------------
; sound_pitch_enverope : (内蔵音源専用)ピッチエンベロープのフレーム処理
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
;	XXX:internal.hへ移動すべきでは
;	XXX:つづり
;
sound_pitch_enverope:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub

	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	sta	$4003,y
.done:
	jsr	pitch_enverope_address	;アドレス一個増やす
	rts


;--------------------
; pitch_sub : ピッチエンベロープのフレーム処理における音源非依存な音量値計算
;
; 入力:
;	x : channel_selx2
;	sound_freq_{low,high,n106},x : エンベロープ処理前の音程
; 出力:
;	sound_freq_{low,high,n106},x : エンベロープ処理の結果が反映される
; 副作用:
;	a : 破壊
;	y : 破壊
;	pitch_add_{low,higi},x : リピートマークを指していた場合には先頭に戻る
;	(以下、freq_add_mcknumberからの間接的な影響)
; 備考:
;	データを読み出した後、アドレス(pitch_add_{low,high},x)を進めないので、
;	呼び出し元で進める必要がある。
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;

pitch_sub:
	; 定義バンク切り替え
	lda	#bank(pitchenve_table)*2
	jsr	change_bank

	ldx	<channel_selx2
	indirect_lda	pitch_add_low
	cmp	#$ff
	beq	.do_repeat

	jmp	freq_add_mcknumber

.do_repeat
	; リピートテーブルを参照してデータアドレスをリストアする
	indirect_lda	pitch_add_low	; XXX:無意味
	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_lp_table,y
	sta	pitch_add_low,x
	lda	pitchenve_lp_table+1,y
	sta	pitch_add_high,x
	jmp	pitch_sub	;XXX:バンク切り替えの後に飛ぶようにすべし


;--------------------
; pitch_enverope_address : ピッチエンベロープアドレスを1つ進める
;
; 入力:
;	x : channel_selx2
;	pitch_add_{low,high},x : 足される前の値
; 出力:
;	pitch_add_{low,high},x : 16bit値として1足される
; 備考:
;	XXX:つづり
;
pitch_enverope_address:
	inc	pitch_add_low,x
	bne	.done
	inc	pitch_add_high,x
.done
	rts


;--------------------
; sound_high_speed_arpeggio : (内蔵音源専用)ノートエンベロープのフレーム処理
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
;	XXX:internal.hへ移動すべきでは
;	XXX:サブルーチン名
;
sound_high_speed_arpeggio:		;note enverope
ARPEGGIO_RETRIG = 0			; 1だとsound_freq_highが変化しなくても書き込む
	.if !ARPEGGIO_RETRIG
	;古いsound_freq_high,xの保存
	lda	sound_freq_high,x
	sta	temporary2
	.endif

	jsr	note_enve_sub
	bcs	.end			; 0なので書かなくてよし

	jsr	frequency_set
	ldx	<channel_selx2
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y
	lda	sound_freq_high,x
	.if !ARPEGGIO_RETRIG
	cmp	temporary2
	beq	.end
	.endif
	sta	$4003,y
	sta	sound_lasthigh,x

.end:
	jsr	arpeggio_address
	rts


;--------------------
; _note_add_set : (note_enve_subの下請け)ノートエンベロープのリピート処理
;
; 備考:
;	XXX:他の_sub系の実装と形式をそろえるべし
;
_note_add_set:
	; 定義バンク切り替え
	lda	#bank(arpeggio_table)*2
	jsr	change_bank

	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_lp_table,y
	sta	arpe_add_low,x
	lda	arpeggio_lp_table+1,y
	sta	arpe_add_high,x
	jmp	note_enve_sub	; XXX:バンク切り替えの後にすべし


;--------------------
; arpeggio_address : ノートエンベロープアドレスを1つ進める
;
; 入力:
;	x : channel_selx2
;	arpe_add_{low,high},x : 足される前の値
; 出力:
;	arpe_add_{low,high},x : 16bit値として1足される
; 備考:
;	XXX:サブルーチン名
;
arpeggio_address:
	inc	arpe_add_low,x
	bne	.done
	inc	arpe_add_high,x
.done:
	rts


;--------------------
; note_enve_sub : ノートエンベロープのフレーム処理における音源非依存な音量値計算
;
; 入力:
;	x : channel_selx2
;	sound_sel,x : エンベロープ処理前のノート番号
; 出力:
;	sound_sel,x : エンベロープ処理の結果が反映される
;	Cフラグ : 1ならば音程変化していない(発声処理を省略できる)
; 副作用:
;	a : 破壊
;	x : channel_selx2
;	y : 破壊
;	t0 : 破壊
;	arpe_add_{low,higi},x : リピートマークを指していた場合には先頭に戻る
;	バンク : arpeggio_tableのあるバンク
; 備考:
;	データを読み出した後、アドレス(arpe_add_{low,high},x)を進めないので、
;	呼び出し元で進める必要がある。
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;
note_enve_sub:
	lda	#bank(arpeggio_table)*2
	jsr	change_bank

	ldx	<channel_selx2
	indirect_lda	arpe_add_low
	cmp	#$ff			;リピートマークかどうか
	beq	_note_add_set

	;変化分がゼロかどうかをチェックする(80hも0として扱う)
	;なお、変化分は補数表現ではなく符号1bit+仮数7bit形式
	cmp	#$00
	beq	.done_by_zero
	cmp	#$80
	beq	.done_by_zero
	bne	.sign_check		;無条件ジャンプ
.done_by_zero:
	sec				;発音処理は不要
	rts

.sign_check:
	eor	#0			;Aレジスタの符号を確認
	bmi	.do_minus		;負数処理へ

	;正数処理
	;ノート番号が不連続なため、1ずつ足してゆく
	;11の次はオクターブ上の0とする
	sta	<t0			;足す値をテンポラリに置く（ループ回数）
.do_loop_plus:
	lda	sound_sel,x		;ノート番号読み出し
	and	#$0f			;下位4bit抽出
	cmp	#$0b			;もし11なら
	beq	.oct_plus		;オクターブ+処理へ
	inc	sound_sel,x		;でなければ音階+1
	jmp	.loop_cond_plus		;ループ条件判定へ
.oct_plus:
	lda	sound_sel,x		;ノート番号読み出し
	and	#$f0			;上位4bit取り出し＆下位4bitゼロ
	clc
	adc	#$10			;オクターブ+1
	sta	sound_sel,x		;ノート番号書き出し
.loop_cond_plus:
	dec	<t0			;ループ回数-1
	lda	<t0			;Zフラグへ反映
	beq	.done			;ゼロなら終了
	bne	.do_loop_plus		;でなければ続行(無条件ジャンプ)

	;負数処理
	;ノート番号が不連続なため、1ずつ引いてゆく
	;0の次はオクターブ下の11とする
.do_minus:
	and	#%01111111		;符号ビットを落とす
	sta	<t0			;引く値をテンポラリに置く（ループ回数）
.do_loop_minus:
	lda	sound_sel,x		;ノート番号読み出し
	and	#$0f			;下位4bit抽出
	beq	.oct_minus		;ゼロならオクターブ−処理へ
	dec	sound_sel,x		;でなければ音階-1
	jmp	.loop_cond_minus	;ループ条件判定へ
.oct_minus:
	lda	sound_sel,x		;ノート番号読み出し
	clc
	adc	#$0b			;音階を11にする(音階部はもともと0)
	sec
	sbc	#$10			;オクターブ-1
	sta	sound_sel,x		;ノート番号書き出し
.loop_cond_minus:
	dec	<t0			;ループ回数-1
	lda	<t0			;Zフラグへ反映
	bne	.do_loop_minus		;ゼロでなければ続行

.done:
	clc				;発音処理が必要
	rts


;--------------------
; effect_init - 音源非依存なキーオン処理(各エフェクトのリセットを伴う)
;
; 入力:
;	x : channel_selx2
;	*_sel,x : 現在選択されている各エフェクト番号
; 出力:
;	*_add_{low,higi},x : 各エフェクトデータの先頭アドレスに初期化
; 副作用:
;	a : 破壊
;	バンク : 変更される
; 備考:
;	実際にレジスタに書き込むのは呼び出し元の仕事。
;	XXX:サブルーチン名。処理内容ではなく「キーオン処理」という用途を反映すべし
;
effect_init:
	;ソフトウェアエンベロープ読み込みアドレス初期化
	lda	#bank(softenve_table)*2
	jsr	change_bank

	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_table,y
	sta	soft_add_low,x
	lda	softenve_table+1,y
	sta	soft_add_high,x

	;ピッチエンベロープ読み込みアドレス初期化
	lda	#bank(pitchenve_table)*2
	jsr	change_bank

	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_table,y
	sta	pitch_add_low,x
	lda	pitchenve_table+1,y
	sta	pitch_add_high,x

	;デューティエンベロープ読み込みアドレス初期化
	lda	#bank(dutyenve_table)*2
	jsr	change_bank

	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x

	;ノートエンベロープ読み込みアドレス初期化
	lda	#bank(arpeggio_table)*2
	jsr	change_bank

	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_table,y
	sta	arpe_add_low,x
	lda	arpeggio_table+1,y
	sta	arpe_add_high,x

	;ソフトウェアLFO初期化
	lda	lfo_start_time,x
	sta	lfo_start_counter,x
	lda	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	lda	lfo_reverse_time,x
	sta	lfo_reverse_counter,x

	.if PITCH_CORRECTION
		;変化方向を表引きして訂正する
		jsr	lfo_initial_vector
	.else
		;内蔵音源かどうかでしか判定しない簡易ルーチン
		lda	<channel_sel
		sec
		sbc	#$04
		bmi	.not_invert

		lda	effect_flag,x
		and	#~EFF_SOFTLFO_VSPEED
		ora	#EFF_SOFTLFO_DIR
		sta	effect_flag,x
		jmp	.invert_done
.not_invert:
		lda	effect_flag,x
		and	#~(EFF_SOFTLFO_VSPEED | EFF_SOFTLFO_DIR)
		sta	effect_flag,x
.invert_done:
	.endif
	;fallthrough

;-----------------------------
; sound_flag_clear_key_on - 音源非依存なキーオン処理(各エフェクトのリセットを伴わない)
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	rest_flag,x : 反映
; 備考:
;	XXX:サブルーチン名。処理内容ではなく「スラー時のキーオン」という用途を反映すべし
;
sound_flag_clear_key_on:
	lda	#RESTF_KEYON ; XXX: rest_flagにフラグが増えたとき破綻する
	sta	rest_flag,x
	rts


;--------------------
; pitchshift_setup : (内蔵音源専用)ピッチシフト/ポルタメントコマンドの処理
;
; 入力:
;	channel_selx2 : チャンネル番号の2倍
;	(xはサブルーチンの先頭でchannel_selx2になる)
;	sound_add_{low,high},x : 現在のサウンドデータアドレス
;		<cmd> <count> <bank> <addr_low> <addr_high>
;		↑ここを指す
; 出力:
; 副作用:
;	a : 破壊
;	x : channel_selx2になる
;	t0 : 破壊
;	t1 : 破壊
;	t2 : 破壊
;	t3 : 破壊
;	t4 : 破壊
;	t5 : 破壊
;	t6 : 破壊
;	ps_temp : 破壊
; 備考:
;

pitchshift_setup:
;この別名のアサインは変更してはならない。特にt0/t1が重要
.diff_lo	= t0
.diff_hi	= t1
.oldfreq_lo	= t2
.oldfreq_hi	= t3
.curfreq_lo	= t4
.curfreq_hi	= t5
.t_note		= t6
.nega_flag	= ps_temp	; XXX:なんでt7を使わないのか
	;現在の周波数レジスタの状態を.curfreqにセーブ
	ldx	<channel_selx2
	lda	sound_freq_low,x
	sta	<.curfreq_lo
	lda	sound_freq_high,x
	sta	<.curfreq_hi

	;直前のコマンドのノート番号に対応する周波数レジスタ値を
	;計算して.oldfreqにセーブ
	jsr	frequency_set
	ldx	<channel_selx2	;XXX:不要?
	lda	sound_freq_low,x
	sta	<.oldfreq_lo
	lda	sound_freq_high,x
	sta	<.oldfreq_hi

	lda	sound_sel,x
	sta	<.t_note

	;コマンドバイトをスキップ
	jsr	sound_data_address

	;備考:続く2バイトには普通の発声コマンドがある
	;音程を読み込む
	lda	[sound_add_low,x]
	sta	sound_sel,x
	sta	ps_nextnote,x

	;音長を読み込む
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	ps_count,x
	sta	sound_counter,x

	jsr	sound_data_address

	;変化量の計算
	lda	sound_sel,x
	cmp	<.t_note
	bne	.ps_calc_freq

	;変化しない場合
	lda	#$00
	sta	ps_step,x
	rts

	;まずは周波数レジスタの変化分を計算する
.ps_calc_freq:
	jsr	frequency_set	; 到達先の音程に対応する周波数レジスタの計算

	ldx	<channel_selx2	;XXX:不要?

	;if ( oldfreq < freq ) goto posi else goto nega
	;これを2バイト比較に展開する
	lda	<.oldfreq_hi
	cmp	sound_freq_high,x
	bcc	.ps_diff_posi	; oldfreq_high < sound_freq_high → posi
	bne	.ps_diff_nega	; oldfreq_high > sound_freq_high → nega
	; oldfreq_high == sound_freq_high
	lda	<.oldfreq_lo
	cmp	sound_freq_low,x
	bcs	.ps_diff_nega	; oldfreq_low >= sound_freq_low → nega

.ps_diff_posi:
	lda	#$00
	sta	<.nega_flag

	;diff = freq - oldfreq
	;これを16bitで行う
	sec
	lda	sound_freq_low,x
	sbc	<.oldfreq_lo
	sta	<.diff_lo

	lda	sound_freq_high,x
	sbc	<.oldfreq_hi
	sta	<.diff_hi
	jmp	.ps_restore_note

.ps_diff_nega:
	lda	#$01
	sta	<.nega_flag

	;diff = oldfreq - freq
	;これを16bitで行う
	sec
	lda	<.oldfreq_lo
	sbc	sound_freq_low,x
	sta	<.diff_lo

	lda	<.oldfreq_hi
	sbc	sound_freq_high,x
	sta	<.diff_hi

.ps_restore_note:
	;現在のノート番号を、一つ前のコマンドのものに戻す
	lda	<.t_note
	sta	sound_sel,x

	;現在の周波数レジスタ値を、このコマンド直前の値に戻す
	lda	<.curfreq_lo
	sta	sound_freq_low,x
	lda	<.curfreq_hi
	sta	sound_freq_high,x

	;傾きの比較
	;if ( diff < count ) goto step else goto freq
	;これを符号無しの16bit vs 8bitで行う
	lda	<.diff_hi
	cmp	#$00
	bcc	.ps_step_base	;XXX:不要。あらゆる値から0を引いても桁借りしないので常にジャンプしない
	bne	.ps_freq_base	;diff_high != 0 → freq
	;diff_high == 0
	lda	<.diff_lo
	cmp	ps_count,x
	bcs	.ps_freq_base	;diff_low > ps_count → freq


	;傾きが1よりもゆるい場合
	;addfreq = 1
	;step = count / diff
.ps_step_base:
	lda	#$01
	sta	ps_addfreq_l,x
	lda	#$00
	sta	ps_addfreq_h,x

	;b_divに渡す16bit値の準備
	;分母 = diff
	;diff(t1:t0)は分子を渡すためのエリアなので先に処理する必要がある
	lda	<.diff_lo
	sta	<t2
	lda	<.diff_hi
	sta	<t3
	;分子 = count
	lda	ps_count,x
	sta	<t0
	lda	#$00
	sta	<t1

	jsr	b_div

	;step = 商
	lda	<t6
	sta	ps_step,x

	jmp	.ps_nega_chk


	;傾きが1よりも急な場合
	;step = 1
	;addfreq = diff / count
.ps_freq_base:
	lda	#$01
	sta	ps_step,x

	;b_divに渡す16bit値の準備
	;分子は既にt1:t0(diff)に代入されている
	;分母 = count
	lda	ps_count,x
	sta	<t2
	lda	#$00
	sta	<t3

	jsr	b_div

	;addfreq = 商
	lda	<t6
	sta	ps_addfreq_l,x
	lda	<t7
	sta	ps_addfreq_h,x

.ps_nega_chk:
	lda	<ps_temp
	beq	.ps_posi

	;二の補数表現での16bit負数にする
	;addfreq = 0 - addfreq
	sec
	lda	#$00
	sbc	ps_addfreq_l,x
	sta	ps_addfreq_l,x

	lda	#$00
	sbc	ps_addfreq_h,x
	sta	ps_addfreq_h,x

.ps_posi:
	lda	ps_step,x
	sta	ps_count,x

	rts


;--------------------
; process_ps : (内蔵音源専用)ピッチシフト/ポルタメントのフレーム処理
;
; 入力:
;	x : channel_selx2
; 副作用:
;	a : 破壊
;	y : 破壊
;	temporary : 破壊
;	ps_count,x : 反映
;	sound_freq_{high,low},x : 反映
; 備考:
;	XXX:internal.hへ移動すべきでは
;	XXX:サブルーチン名
;
process_ps:
	;ps_step,xが0なら無効
	lda	ps_step,x
	beq	.done

	;ウェイト : ps_count,xを1つ減らし、0になっていなければ終了
	dec	ps_count,x
	bne	.done

	;ウェイトカウントリセット
	lda	ps_step,x
	sta	ps_count,x

	;古いsound_freq_high,xを保存
	lda	sound_freq_high,x
	sta	temporary

	;freq += ps_addfreq
	clc
	lda	sound_freq_low,x
	adc	ps_addfreq_l,x
	sta	sound_freq_low,x
	lda	sound_freq_high,x
	adc	ps_addfreq_h,x
	sta	sound_freq_high,x

	;音程レジスタへの書き込み
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y

	;sound_freq_high,xが変化していなければ上位バイトは書き込まない
	lda	sound_freq_high,x
	cmp	temporary
	beq	.done
	sta	$4003,y
	sta	sound_lasthigh,x

.done:
	rts


;--------------------
; b_div : 符号無し16ビット除算 (C = A / B)
;
; 入力:
;	t1:t0 : 被除数(A)
;	t3:t2 : 除数(B)
; 出力:
;	t7:t6 : 商(C)
;	t1:t0 : 剰余(のはず/今のところ使われていない)
; 副作用:
;	t4 : 破壊
;	t5 : 破壊
; 備考:
;	XXX:サブルーチン名
;
b_div:
;この別名はそのまま入出力に対応するため変更してはならない
.A_lo	= t0
.A_hi	= t1
.B_lo	= t2
.B_hi	= t3
.tmp_lo	= t4
.tmp_hi	= t5
.C_lo	= t6
.C_hi	= t7
	;quotient = 0
	lda  #$00
	sta  <.C_lo
	sta  <.C_hi

	;tmp = 1
	lda  #$01
	sta  <.tmp_lo
	lda  #$00
	sta  <.tmp_hi

	;ゼロ除算の検査
	lda  <.B_lo
	ora  <.B_hi
	bne  .do_phase1
	rts

.do_phase1:
	;フェーズ1
	;  B*2^(n+1) > A >= B*2^n となるようなnを探し、
	;  B = B*2^n と tmp = 2^n を得る
	;  while (!(B & 0x8000) && (B<<1) <= A) {
	;    B <<= 1;
	;    tmp <<= 1;
	;  }
	;  以下の実装では、次のようにBを使いまわしている
	;  while (!(B & 0x8000)) {
	;    B <<= 1;
	;    if (B > A) {
	;      B >>= 1;
	;      break;
	;    }
	;    tmp <<= 1;
	;  }
	;  XXX:フェーズ2ともども、A == Bになったときに中断したほうが
	;  いいのかもしれないし、トータルではよくないのかもしれない
.phase1_loop:
	lda  <.B_hi
	and  #$80
	bne  .phase1_done

	;divider <<= 1
	asl  <.B_lo
	rol  <.B_hi

	; if ( A < B ) goto break else goto next
	lda	<.A_hi
	cmp	<.B_hi
	bcc	.phase1_break	; (A < B)
	bne	.phase1_next	; (A > B)
	lda	<.A_lo
	cmp	<.B_lo
	bcc	.phase1_break	; (A < B)

.phase1_next:
	;tmp <<= 1
	asl  <.tmp_lo
	rol  <.tmp_hi
	jmp  .phase1_loop

.phase1_break:
	;B >>= 1
	lsr  <.B_hi
	ror  <.B_lo
.phase1_done:

.do_phase2:
	;フェーズ2
	;  二進で一桁ずつ引けるかどうか調べ、引けるなら商に加算する。
	;  以下と等価
	;  while (tmp != 0) {
	;    if (A < B) {
	;      B >>= 1;
	;      tmp >>= 1;
	;    } else {
	;      A -= B;
	;      C += tmp;
	;    }
	;  }
	;  以下の実装では、フェーズ1の結果などを踏まえて、
	;  評価順序を変えて次のように最適化されている
	;  do {
	;    A -= B;
	;    C += tmp;
	;    do {
	;      B >>= 1;
	;      tmp >>= 1;
	;      if (tmp == 0)
	;        goto done;
	;    } while (B > A);
	;  } while (true);
	;  done:;
.phase2_outerloop:
	;A -= B
	lda  <.A_lo
	sec
	sbc  <.B_lo
	sta  <.A_lo
	lda  <.A_hi
	sbc  <.B_hi
	sta  <.A_hi

	;C += tmp
	clc
	lda  <.C_lo
	adc  <.tmp_lo
	sta  <.C_lo
	lda  <.C_hi
	adc  <.tmp_hi
	sta  <.C_hi

.phase2_innerloop:
	;B >>= 1
	lsr  <.B_hi
	ror  <.B_lo

	;tmp >>= 1
	lsr  <.tmp_hi
	ror  <.tmp_lo
	bcs  .phase2_done

	; if ( A < B ) goto innerloop else goto outerloop
	lda	<.A_hi
	cmp	<.B_hi
	bcc	.phase2_innerloop	; (dividend < divider)
	bne	.phase2_outerloop	; (dividend > divider)
	lda	<.A_lo
	cmp	<.B_lo
	bcc	.phase2_innerloop	; (dividend < divider)
	jmp	.phase2_outerloop

.phase2_done:
.done:
	rts
