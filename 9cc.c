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

typedef enum
{
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NUM,      // 整数トークン
}NodeKind;

typedef struct TokenDummy Token;
typedef struct NodeDummy Node;

struct TokenDummy
{
	TokenKind kind;
	Token *next;
	long int num;
	char *str;      // トークン文字列
};

struct NodeDummy
{
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	long int num;
};

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// エラーを報告するための関数
void error_at(char *loc, char* fmt, ...)
{
	va_list args;
	va_start( args, fmt );

	int pos = loc - user_input;

	fprintf( stderr, "%s\n", user_input );
	fprintf( stderr, "%*s", pos, " " ); // pos個の空白を表示
	fprintf( stderr, "^ " );
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
	user_input += 1;
	return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char op)
{
	if(token->kind != TK_RESERVED || token->str[0] != op)
		error_at(token->str, "'%c'ではありません\n", op );
	token = token->next;
	user_input += 1;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
long int expect_number()
{
	if( token->kind != TK_NUM )
		error_at(token->str, "数ではありません\n");
	long int val = token->num;
	token = token->next;

	char cval[4];
	sprintf(cval, "%ld", val);
	user_input += strlen(cval);
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

		if( *p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' )
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

		error_at(token->str, "トークナイズできません");
	}

	// cur->nextを設定
	new_token(TK_EOF, cur, p);

	return head.next;
}

// 新しい記号ノードの作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
	Node *node = calloc(1, sizeof(Node));

	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;

	return node;
}

// 新しい数値ノードの作成
Node *new_node_num(long int num)
{
	Node* node = calloc(1, sizeof(Node));

	node->kind = ND_NUM;
	node->num = num;

	return node;
}

Node *expr();
Node *mul();
Node *unary();
Node *primary();

// primary = num | "(" expr ")"
Node* primary()
{
	if(consume('('))
	{
		Node *node = expr();
		expect(')');
		return node;
	}

	return new_node_num(expect_number());
}

// unary = ("+" | "-")? primary
Node *unary()
{
	if(consume('+')){}
	else if(consume('-'))
		return new_node(ND_SUB, new_node_num(0), primary());

	return primary();
}

// mul = unary ("*" unary | "/" unary)*
Node* mul()
{
	Node* node = unary();

	for(;;)
	{
		if(consume('*'))
			node = new_node(ND_MUL, node, mul());
		else if(consume('/'))
			node = new_node(ND_DIV, node, mul());
		else
			return node;
	}
}

// expr = mul ("+" mul | "-" mul)*
Node *expr()
{
	Node *node = mul();

	for(;;)
	{
		if(consume('+'))
			node = new_node(ND_ADD, node, mul());
		else if(consume('-'))
			node = new_node(ND_SUB, node, mul());
		else
			return node;
	}
}

void gen(Node* node)
{
	if(node->kind == ND_NUM)
	{
		printf("	mov rdx, %ld\n", node->num);
		printf("	push rdx\n");
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch(node->kind)
	{
	case ND_ADD: printf("	add rax, rdi\n"); break;
	case ND_SUB: printf("	sub rax, rdi\n"); break;
	case ND_MUL: printf("	mul rdi\n"); break;
	case ND_DIV:
		printf("	cqo\n");
		printf("	idiv rdi\n"); break;
	}

	printf("	push rax\n");
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
	user_input = argv[1];

	Node* node = expr();

	gen(node);

	printf("	pop rax\n");
	printf( "	ret\n");
	return 0;
}
