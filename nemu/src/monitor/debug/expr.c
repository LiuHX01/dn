#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, DECNUM

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority; // the precedence level of rules
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE, 0},					// spaces
	{"\\+", '+', 4},					// plus
	{"==", EQ, 3},						// equal
	{"-", '-', 4},						// sub
	{"\\*", '*', 5},					// multiply
	{"/", '/', 5},						// division
	{"\\(", '(', 7},					// left bracket
	{"\\)", ')', 7},					// right bracket
	{"[0-9]+", DECNUM, 0}			// Decimal
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE: break;
					default:
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
					// default: panic("please implement me");	
				}
				break;
			}
		}
		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parentheses(int l, int r) {
	int i;
	if (tokens[l].type == '(' && tokens[r].type == ')') {
		int lc = 0, rc = 0;
		for (i = l + 1; l < r; i++) {
			if (tokens[i].type == '(')
				lc++;
			if (tokens[i].type == ')')
				rc++;
			if (lc < rc)
				return false;
		}
		if (lc == rc)
			return true;
	}
	return false;
}

int dominant_operator(int l, int r) {
	int op = 1;
	int i, j;
	int minn = 20;
	for (i = l; i <= r; i++) {
		if (tokens[i].type == DECNUM) 
			continue;
		int bnum = 0, k = 1;
		for (j = i - 1; j >= l; j--) { // right to left
			if (tokens[j].type == '(') {
				if (bnum == 0) { // check legality of brackets 
					k = 0;
					break;
				}
				else bnum--;
			}
			if (tokens[j].type == ')')
				bnum++;
		}
		if (k == 0)
			continue;
		if (tokens[i].priority <= minn) { // find the minimum weight
			minn = tokens[i].priority;
			op = i;
		}
	}
	return op;
}

unsigned int eval(int l, int r) {
	if (l > r) {
		printf("Wrong! because l > r");
		return 0;
	}
	
	else if (l == r) {
		int num = 0;
		if (tokens[l].type == DECNUM)
			sscanf(tokens[l].str, "%d", &num);
		return num;
	}
	
	else if (check_parentheses(l, r) == true) {
		return eval(l+1, r-1);
	}
	
	else {
		int op = dominant_operator(l, r);
		int val1 = eval(l, op-1);
		int val2 = eval(op+1, r);
		switch (tokens[op].type) {
			case '+': return val1 + val2;
			case '-': return val1 - val2;
			case '*': return val1 * val2;
			case '/': return val1 / val2;
			default: break;
		}
	}
	assert(1);
	return 2;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	*success = true;
	return eval(0, nr_token-1);

	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	//return 0;
}

