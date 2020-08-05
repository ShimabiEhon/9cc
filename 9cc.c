#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum
{
	TK_RESERVED, // 記号
	TK_NUM,      // 整数トークン
	TK_EOF       // 入力の終わりを表すトークン
}TokenKind;

typedef struct TokenDummy Token;

struct TokenDummy
{
	TokenKind kind;
	Token *next;
	long int num;
	char *str;      // トークン文字列
};

// 現在着目しているトークン
Token* token;

// エラーを報告するための関数
// printf と同じ引数を取る
void error( char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );

	vfprintf( stderr, fmt, args );
	fprintf( stderr, "\n" );
	exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char op)
{
	if( token->kind != TK_RESERVED || token->str[0] != op )
		return false;
	token = token->next;
	return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op)
{
	if( token->kind != TK_RESERVED || token->str[0] != op )
		error( "'%c'ではありません\n", op );
	token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long int expect_number()
{
	if( token->kind != TK_NUM )
		error("数ではありません\n");
	long int val = token->num;
	token = token->next;
	return val;
}

bool at_eof()
{
	return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str)
{
	Token* tok = calloc( 1, sizeof(Token) );
	tok->kind = kind;
	tok->str = str;

	cur->next = tok;
	return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p)
{
	Token head;
	head.next = NULL;
	Token* cur = &head;

	while( *p )
	{
		if( isspace( *p ) )
		{
			p++;
			continue;
		}

		if( *p == '+' || *p == '-' )
		{
			// cur->nextを設定して、それをcurに代入している
			cur = new_token( TK_RESERVED, cur, p );

			p++;
			continue;
		}
		if( isdigit(*p) )
		{
			// cur->nextを設定して、それをcurに代入している
			cur = new_token( TK_NUM, cur, p );

			cur->num = strtol( p, &p, 10 );

			continue;
		}

		error("トークナイズできません");
	}

	// cur->nextを設定
	new_token(TK_EOF, cur, p);

	return head.next;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}

	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");

	token = tokenize(argv[1]);

	printf("	mov rax, %ld\n", expect_number() );

	while( !at_eof() )
	{
		if( consume('+') ) // token->str[0]が'+'の時はconsume関数内でtokenを進めている
		{
			printf("	add rax, %ld\n", expect_number() ); // expect_number関数内でもtokenを進めている
			continue;
		}

		expect('-'); // token->str[0]が'-'の時はexpect関数内でtokenを進めている
		printf("	sub rax, %ld\n", expect_number() ); // expect_number関数内でもtokenを進めている
	}

	printf( "	ret\n");
	return 0;
}
