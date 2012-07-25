;-----------------------------------------------------------------------
;2a03 squ tri noise driver
;-----------------------------------------------------------------------
sound_internal:
	ldx	<channel_selx2
	dec	sound_counter,x		;�J�E���^���������炵
	beq	.sound_read_go		;�[���Ȃ�T�E���h�ǂݍ���
	jsr	do_effect		;�[���ȊO�Ȃ�G�t�F�N�g����
	rts				;�����
.sound_read_go
	jsr	sound_data_read
	jsr	do_effect
	lda	rest_flag,x
	and	#%00000010		;�L�[�I���t���O
	beq	.end1			
	jsr	sound_data_write	;�����Ă�����f�[�^�����o��
	lda	rest_flag,x
	and	#%11111101		;�L�[�I���t���O�I�t
	sta	rest_flag,x
.end1
	rts

;-------
do_effect:
	lda	rest_flag,x
	and	#%00000001
	beq	.duty_write2
	rts				;�x���Ȃ�I���

.duty_write2:
	lda	effect_flag,x
	and	#%00000100
	beq	.enve_write2
	jsr	sound_duty_enverope

.enve_write2:
	lda	effect_flag,x
	and	#%00000001
	beq	.lfo_write2
	jsr	sound_software_enverope

.lfo_write2:
	lda	effect_flag,x
	and	#%00010000
	beq	.ps_process
	jsr	sound_lfo

.ps_process:
	jsr	process_ps

.pitchenve_write2:
	lda	effect_flag,x
	and	#%00000010
	beq	.arpeggio_write2
	jsr	sound_pitch_enverope

.arpeggio_write2:
	lda	effect_flag,x
	and	#%00001000
	beq	.return7
	lda	rest_flag,x		;�L�[�I���̂Ƃ��Ƃ����łȂ��Ƃ��ŃA���y�W�I�̋����͂�����
	and	#%00000010		;�L�[�I���t���O
	bne	.arpe_key_on
	jsr	sound_high_speed_arpeggio	;�L�[�I������Ȃ��Ƃ��ʏ�͂���
	jmp	.return7
.arpe_key_on				;�L�[�I���������̏ꍇ
	jsr	note_enve_sub		;���������������ŁA�����ł͏������݂͂��Ȃ�
	jsr	frequency_set
	jsr	arpeggio_address
.return7:
	rts

;-------------------------------------------------------------------------------
;register write sub routines
;-------------------------------------------------------------------------------
frequency_set:

	ldx	<channel_selx2
	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	and	#%00001111		;����4bit�����o����
	asl	a
	tay

	lda	<channel_sel
	cmp	#$03
	beq	noise_frequency_get	;�S�`�����l���ڂȂ�m�C�Y���g���擾��

	lda	psg_frequency_table,y	;PSG���g���e�[�u������Low��ǂݏo��
	sta	sound_freq_low,x	;��������
	lda	psg_frequency_table+1,y	;PSG���g���e�[�u������High��ǂݏo��
	sta	sound_freq_high,x	;��������

oct_set1:

	lda	sound_sel,x		;���K�f�[�^�ǂݏo��
	lsr	a			;���4bit�����o��
	lsr	a			;
	lsr	a			;
	lsr	a			;
	beq	freq_end		;�[���Ȃ炻�̂܂܏I���
	tay				;

oct_set2:

	lsr	sound_freq_high,x	;�E�V�t�g�@������C��
	ror	sound_freq_low,x	;C���玝���Ă���ł�@�E���[�e�C�g
	dey				;
	bne	oct_set2		;�I�N�^�[�u���J��Ԃ�

freq_end:
	jsr	detune_write_sub
	rts


noise_frequency_get:
	lda	noise_frequency_table,y	;���g���e�[�u������Low��ǂݏo��
	sta	sound_freq_low,x	;��������
	jsr	detune_write_sub
	lda	#$00			;$400F�͏��0
	sta	sound_freq_high,x	;��������
	rts

;-----------------------
sound_data_write:
	ldx	<channel_selx2
	ldy	<channel_selx4
	
	lda	register_low,x		;���ʕێ�
	ora	register_high,x
	sta	$4000,y
	lda	sound_freq_low,x	;Low Write
	sta	$4002,y

	lda	effect2_flags,x
	and	#%00000010
	bne	sound_write_smooth
	lda	sound_freq_high,x	;High Write
	sta	$4003,y
	sta	sound_lasthigh,x
	rts

sound_write_smooth:
	lda	sound_freq_high,x	;High Write
	cmp	sound_lasthigh,x
	beq	sound_data_skip_high
	sta	$4003,y
	sta	sound_lasthigh,x
sound_data_skip_high:
	rts

;-------------------------------------------------------------------------------
;command read routine
;-------------------------------------------------------------------------------
sound_data_read:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]
;----------
;���[�v����1
loop_program
	cmp	#$a0
	bne	loop_program2
	jsr	loop_sub
	jmp	sound_data_read
;----------
;���[�v����2(����)
loop_program2
	cmp	#$a1
	bne	bank_command		;duty_set
	jsr	loop_sub2
	jmp	sound_data_read
;----------
;�o���N�؂�ւ�
bank_command
	cmp	#$ee
	bne	int_hwenv_command
	jsr	data_bank_addr
	jmp	sound_data_read
;----------
;�f�[�^�G���h�ݒ�
;data_end:
;	cmp	#$ff
;	bne	duty_set
;	jsr	data_end_sub
;	jmp	sound_data_read

;----------
;�n�[�h�G���x���[�v
int_hwenv_command:
	cmp	#$f0
	bne	slur_command

	jsr	sound_data_address
	lda	effect2_flags,x
	and	#%11001111
	ora	[sound_add_low,x]
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_data_read


;----------
;�X���[
slur_command:
	cmp	#$e9
	bne	smooth_command
	lda	effect2_flags,x
	ora	#%00000001
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;�X���[�X
smooth_command:
	cmp	#$e8
	bne	pitchshift_command
	jsr	sound_data_address
	lda	[sound_add_low,x]
	beq	.smooth_off
.smooth_on
	lda	effect2_flags,x
	ora	#%00000010
	sta	effect2_flags,x
	jmp	.smooth_fin
.smooth_off:
	lda	effect2_flags,x
	and	#%11111101
	sta	effect2_flags,x
.smooth_fin:
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;�s�b�`�V�t�g
pitchshift_command:
	cmp	#$e7
	bne	duty_set
	lda	ps_step,x
	beq	.ps_setup

	lda	ps_nextnote,x    ; PS�R�}���h�Ɏg�p�����m�[�g����ɂ���
	sta	sound_sel,x

.ps_setup
	jmp	pitchshift_setup

;----------
;���F�ݒ�
duty_set:
	cmp	#$fe
	bne	volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;���F�f�[�^�ǂݏo��
	pha
	bpl	duty_enverope_part	;�a���[�e�B�G���x������


; register_high = ���4bit�A�ꎞ�ޔ��Ƃ��ė��p


duty_select_part:
	lda	effect_flag,x
	and	#%11111011
	sta	effect_flag,x		;�f���[�e�B�G���x���[�v�����w��

	lda	effect2_flags,x         ; hw_envelope
	and	#%00110000
	eor	#%00110000
	sta	register_high,x

	pla
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
;	ora	#%00110000		;hardware envelope & ... disable
	ora	register_high,x		;hw_envelope
	sta	register_high,x		;��������
	ora	register_low,x
	ldy	<channel_selx4
	sta	$4000,y
	jsr	sound_data_address
	jmp	sound_data_read

duty_enverope_part:
	lda	effect_flag,x
	ora	#%00000100
	sta	effect_flag,x		;�f���[�e�B�G���x���[�v�L���w��
	pla
	sta	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y	;�f���[�e�B�G���x���[�v�A�h���X�ݒ�
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	sound_data_read

;----------
;���ʐݒ�
volume_set:
	cmp	#$fd
	bne	rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	softenve_part		;�\�t�g�G���x������

volume_part:
	lda	effect_flag,x
	and	#%11111110
	sta	effect_flag,x		;�\�t�g�G���x�����w��

	lda	temporary
	and	#%00001111
	sta	register_low,x
	ora	register_high,x
	ldy	<channel_selx4
	sta	$4000,y			;�{�����[����������
	jsr	sound_data_address
	jmp	sound_data_read

softenve_part:
	jsr	volume_sub
	jmp	sound_data_read
;----------
rest_set:
	cmp	#$fc
	bne	lfo_set

	lda	rest_flag,x
	ora	#%00000001
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	ldy	<channel_selx4
	lda	<channel_sel
	cmp	#$02
	beq	tri

	lda	register_high,x
	sta	$4000,y
	jsr	sound_data_address
	rts
tri:
	lda	#$00
	sta	$4000,y
	jsr	sound_data_address
	rts
;----------
lfo_set:
	cmp	#$fb
	bne	detune_set
	jsr	lfo_set_sub
	jmp	sound_data_read

;----------
detune_set:
	cmp	#$fa
	bne	sweep_set
	jsr	detune_sub
	jmp	sound_data_read

;----------
sweep_set:
	cmp	#$f9
	bne	pitch_set

	jsr	sound_data_address
	lda	[sound_add_low,x]
	ldy	<channel_selx4
	sta	$4001,y
	jsr	sound_data_address
	jmp	sound_data_read
;----------
;�s�b�`�G���x���[�v�ݒ�
pitch_set:
	cmp	#$f8
	bne	arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_data_read

;----------
;�m�[�g�G���x���[�v�ݒ�
arpeggio_set:
	cmp	#$f7
	bne	freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_data_read

;----------
;�Đ����g�����ڐݒ�
freq_direct_set:
	cmp	#$f6
	bne	y_command_set
	jsr	direct_freq_sub
	rts

;----------
;���R�}���h�ݒ�
y_command_set:
	cmp	#$f5
	bne	wait_set
	jsr	y_sub
	jmp	sound_data_read

;----------
;�E�F�C�g�ݒ�
wait_set:
	cmp	#$f4
	bne	oto_set
	jsr	wait_sub
	rts
;----------
oto_set:
	sta	sound_sel,x		;�����͂܂����

	lda	#$00
	sta	ps_step,x

	jsr	sound_data_address
	lda	[sound_add_low,x]	;�����ǂݏo��
	sta	sound_counter,x		;���ۂ̃J�E���g�l�ƂȂ�܂�
	jsr	sound_data_address

	jsr	frequency_set		;���g���Z�b�g��
	lda	effect2_flags,x		;�X���[�t���O�̃`�F�b�N
	and	#%00000001
	beq	no_slur
	lda	effect2_flags,x
	and	#%11111110
	sta	effect2_flags,x		;�X���[�t���O�̃N���A
	jmp	sound_flag_clear_key_on

no_slur:
	jmp	effect_init


