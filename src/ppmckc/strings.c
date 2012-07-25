/************************************************************/
/*															*/
/************************************************************/
#include	<stddef.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>


/*--------------------------------------------------------------
	�X�y�[�X�^�^�u�̃X�L�b�v
 Input:
	char	*ptr		:�f�[�^�i�[�|�C���^
 Output:
	char	*ptr		;�X�L�b�v��̃|�C���^
--------------------------------------------------------------*/
char *skipSpaceOld( char *ptr )
{
	while( *ptr != '\0' ) {
		if( *ptr != ' ' && *ptr != '\t' ) {
			break;
		}
		ptr++;
	}
	return ptr;
}


/*--------------------------------------------------------------
	������̃X�L�b�v
--------------------------------------------------------------*/

char *skipQuote( char *ptr )
{
	if (*ptr && 
	    *ptr == '\"')
	{
		ptr++; // skip start charactor
		while( *ptr )
		{
			if ( *ptr == '\"') // end of the quote
			{
				ptr++; break;
			}

			if ( *ptr == '\\' && *( ptr+1 ) ) // skip Escape
				ptr++;
			ptr++;
		}
	}
	return ptr;
}

/*--------------------------------------------------------------
	�R�����g�����̃`�F�b�N
--------------------------------------------------------------*/
int  isComment( char *ptr )
{
	if (*ptr && 
	    ( *ptr == ';' ||
//	   (*ptr == '/' && *(ptr+1) == '/') )
	    *ptr == '/' ) )
		return 1;

	return 0;
}

/*--------------------------------------------------------------
	�R�����g�̃X�L�b�v
--------------------------------------------------------------*/
char *skipComment( char *ptr )
{
	if (isComment(ptr))
	{
		while(1) 
		{
			// '\0' = EOL or EOF , '\n' = EOL
			if (*ptr == '\0' || *ptr == '\n') 
				break;
			ptr++;
		}
	}
	return  ptr;
}


/*--------------------------------------------------------------
	�X�y�[�X�^�^�u�̃X�L�b�v(�s�R�����g����΂�)
--------------------------------------------------------------*/
char *skipSpace( char *ptr )
{
	while (1) {
		if (*ptr == '\0') break; //EOL or EOF
		if (*ptr == ' ' || *ptr == '\t') {
			//Skip Space
			ptr++;
			continue;
		} else if ( isComment(ptr) ) {
			//Skip Comment(return EOL)
			ptr = skipComment( ptr );
		} else {
			//Normal Chars
			break;
		}
	}
	return ptr;
}




/*----------------------------------------------------------
	�������������ǂ����̃`�F�b�N
 Input:
	char	c	: ����
 Return:
	0:�����ȊO 1: �����R�[�h
----------------------------------------------------------*/
int checkKanji( unsigned char c )
{
	if( 0x81 <= c && c <= 0x9f ) return 1;
	if( 0xe0 <= c && c <= 0xef ) return 1;
	return 0;
}



/*----------------------------------------------------------
	�������啶���ɂ���(�����Ή���)
 Input:
	char *ptr	: ������ւ̃|�C���^
 Output:
	none
----------------------------------------------------------*/
void strupper( char *ptr )
{
	while( *ptr != '\0' ) {
		if( checkKanji( (unsigned char)*ptr ) == 0 ) {
			*ptr = toupper( (int)*ptr );
			ptr++;
		} else {
			/* �����̎��̏��� */
			ptr += 2;
		}
	}
}




/*--------------------------------------------------------------
	������𐔒l�ɕϊ�
 Input:

 Output:

--------------------------------------------------------------*/
int Asc2Int( char *ptr, int *cnt )
{
	int		num;
	char	c;
	int		minus_flag = 0;

	num = 0;
	*cnt = 0;

	if( *ptr == '-' ) {
		minus_flag = 1;
		ptr++;
		(*cnt)++;
	}
	switch( *ptr ) {
	/* 16�i�� */
	  case 'x':
	  case '$':
		ptr++;
		(*cnt)++;
		while( 1 ) {
			c = toupper( *ptr );
			if( '0' <= c && c <= '9' ) {
				num = num * 16 + (c-'0');
			} else if( 'A' <= c && c <= 'F' ) {
				num = num * 16 + (c-'A'+10);
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
		break;
	/* 2�i�� */
	  case '%':
		ptr++;
		(*cnt)++;
		while( 1 ) {
			if( '0' <= *ptr && *ptr <= '1' ) {
				num = num * 2 + (*ptr-'0');
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
	/* 10�i�� */
	  default:
		while( 1 ) {
			if( '0' <= *ptr && *ptr <= '9' ) {
				num = num * 10 + (*ptr-'0');
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
	}
	if( minus_flag != 0 ) {
		num = -num;
	}
	return num;
}

