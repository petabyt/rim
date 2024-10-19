#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum TokenType {
	TOK_SLASH = '/',
	TOK_ANGLE_LEFT = '<',
	TOK_ANGLE_RIGHT = '>',
	TOK_END_ANGLE_LEFT = '{',
	TOK_TEXT = 't',
	TOK_BRACE_LEFT = '{',
	TOK_BRACE_RIGHT = '}',
	TOK_EOF = '0',
};

struct Token {
	enum TokenType type;
	int text_of;
	int text_length;
};

struct Context {
	char *file;
	int of;
};

void emit_push_widget(const char *tag) {}
void emit_set_widget_text(int sp, const char *text) {}
void emit_add_widget_to(int sp, int to) {}
void emit_pop() {}

struct Token next_token(struct Context *ctx) {
	int of = ctx->of;
	int curr = ctx->of;

	struct Token tok;

	if (ctx->file[curr] == '<') {
		tok = (struct Token){
			.type = TOK_ANGLE_LEFT,
			.text_of = of,
			.text_length = 1,
		};
		curr++;
		goto found;
	} else if (ctx->file[curr] == '>') {
		tok = (struct Token){
			.type = TOK_ANGLE_RIGHT,
			.text_of = of,
			.text_length = 1,
		};
		curr++;
		goto found;
	} else if (ctx->file[curr] == '/') {
		tok = (struct Token){
			.type = TOK_SLASH,
			.text_of = of,
			.text_length = 1,
		};
		curr++;
		goto found;
	} else if (ctx->file[curr] == '\0') {
		tok = (struct Token){
			.type = TOK_EOF,
			.text_of = of,
			.text_length = 0,
		};
		goto found;
	}

	while (ctx->file[curr] != '\0') {
		// ... process backslash, unicode ...
		if (ctx->file[curr] == '<' || ctx->file[curr] == '>') {
			tok = (struct Token){
				.type = TOK_TEXT,
				.text_of = of,
				.text_length = curr - of,
			};
			goto found;
		}
		curr++;
	}

	found:;
	ctx->of = curr;
	return tok;
}

char *str_from_tok(struct Context *ctx, struct Token *tok) {
	char *string = malloc(tok->text_length + 1);
	strncpy(string, ctx->file + tok->text_of, tok->text_length);
	string[tok->text_length] = '\0';
	return string;
}

enum Codes {
	PARSE_OK = 0,
	PARSE_END_OF_TAG,
	PARSE_ERR = 0,
};

int parse_tag(struct Context *ctx);

int parse_tag(struct Context *ctx) {
	struct Token tok_text = next_token(ctx);
	if (tok_text.type == TOK_SLASH) {
		return PARSE_END_OF_TAG;
	}
	if (tok_text.type != TOK_TEXT) {
		printf("text: %s\n", ctx->file + tok_text.text_of - 2);
		printf("Expected %c, got %c\n", TOK_TEXT, tok_text.type);
		assert(tok_text.type == TOK_TEXT);
		return 1;
	}

	char *string = str_from_tok(ctx, &tok_text);
	printf("push '%s' to stack\n", string);
	tok_text = next_token(ctx);
	assert(tok_text.type == TOK_ANGLE_RIGHT);
	if (tok_text.type != TOK_ANGLE_RIGHT) {
		printf("Expected %c, got %c\n", TOK_ANGLE_RIGHT, tok_text.type);
		return 1;
	}

	while (1) {
		struct Token tok = next_token(ctx);
	
		if (tok.type == TOK_ANGLE_LEFT) {
			int rc = parse_tag(ctx);
			if (rc == PARSE_END_OF_TAG) {
				break;
			}
		} else if (tok.type == TOK_TEXT) {
			char *string = str_from_tok(ctx, &tok);
			printf("set widget -1 text to \"%s\"\n", string);
		} else if (tok.type == TOK_EOF) {
			break;
		} else {
			printf("Unexpected %c", tok.type);
			return PARSE_ERR;
		}
	}

	tok_text = next_token(ctx);
	assert(tok_text.type == TOK_TEXT);
	if (tok_text.type != TOK_TEXT) {
		printf("Expected %c, got %c\n", TOK_TEXT, tok_text.type);
		return 1;
	}

	string = str_from_tok(ctx, &tok_text);
	printf("add widget -1 '%s' child to 0 (root)\n", string);
	printf("pop\n");
	tok_text = next_token(ctx);
	assert(tok_text.type == TOK_ANGLE_RIGHT);
	if (tok_text.type != TOK_ANGLE_RIGHT) {
		printf("Expected %c, got %c\n", TOK_ANGLE_RIGHT, tok_text.type);
		return 1;
	}

	return 0;
}

int parse(struct Context *ctx) {
	while (1) {
		struct Token tok = next_token(ctx);
		if (tok.type == TOK_ANGLE_LEFT) {
			if (parse_tag(ctx)) return 1;
		} else if (tok.type == TOK_TEXT) {
			char *string = str_from_tok(ctx, &tok);
			printf("add plaintext: %s\n", string);
			continue;
		} else if (tok.type == TOK_EOF) {
			break;
		}
	}

	return 0;
}

int main() {
	FILE *f = fopen("basic.svelte", "rb");
	if (!f) return -1;

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);

	char *buf = malloc(size + 10);
	if (fread(buf, 1, size, f) != size) {
		fclose(f);
		free(buf);
		return -1;
	}
	buf[size] = '\0';

	fclose(f);

	struct Context ctx = {
		.file = buf,
		.of = 0,
	};

	parse(&ctx);

	return 0;
}
