#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, DECNUM, REGISTER, HEXNUM, AND, OR, NEQ, NEGATIVE, DEREFERENCE

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

	// the higher the weight , the higher the priority.
	{" +",	NOTYPE, 0},					// spaces
	{"\\+", '+', 4},					// plus
	{"==", EQ, 3},						// equal
	{"-", '-', 4},						// sub
	{"\\*", '*', 5},					// multiply
	{"/", '/', 5},						// division
	{"\\(", '(', 7},					// left bracket
	{"\\)", ')', 7},					// right bracket
	{"\\b[0-9]+\\b", DECNUM, 0},		// Decimal
	{"\\0x[0-9a-fA-F]+", HEXNUM, 0},	// Hexadecimal
	{"\\$[a-z]+", REGISTER, 0},			// register
	{"&&", AND, 2},						// and
	{"\\|\\|", OR, 1},					// or
	{"!=", NEQ, 3},						// not equal
	{"!", '!', 6},						// not
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
				char *substr_start_register = e + position + 1;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE: break;
					case REGISTER: // neglect $
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority;
						strncpy(tokens[nr_token].str, substr_start_register, substr_len-1);
						tokens[nr_token].str[substr_len-1] = '\0';
						nr_token++;
						break;
					default:
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority;
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
						//printf("%d\n", nr_token);
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
/**
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
}*/

bool check_parentheses (int l,int r)
{
	int i;
	if (tokens[l].type == '(' && tokens[r].type ==')')
	{
		int lc = 0, rc = 0;
		for (i = l + 1; i < r; i ++)
		{
			if (tokens[i].type == '(')lc ++;
			if (tokens[i].type == ')')rc ++;
			if (rc > lc)return false;	
		}
		if (lc == rc)return true;
	}
	return false;
}


int dominant_operator(int l, int r) {
	int op = 1;
	int i, j;
	int minn = 100;
	for (i = l; i <= r; i++) {
		if (tokens[i].type == DECNUM || tokens[i].type == HEXNUM || tokens[i].type == REGISTER) 
			continue;
		int bnum = 0, k = 1;
		for (j = i - 1; j >= l; j--) { // right to left
			if (tokens[j].type == '(' && !bnum) {
				k = 0;
				break;
			}
			if (tokens[j].type == '(')
				bnum--;
			if (tokens[j].type == ')')
				bnum++;
		}
		if (!k)
			continue;
		if (tokens[i].priority <= minn) { // find the minimum weight
			minn = tokens[i].priority;
			op = i;
		}
	}
	//printf("%d\n", op);
	return op;
}


uint32_t eval(int l, int r) {
	//printf("l=%d,r=%d\n", l, r);
	if (l > r) {
		//printf("l=%d,r=%d\n", l, r);
		printf("wrong!!!\n");
		return 0;
	}
	
	if (l == r) {
		uint32_t num = 0;
		if (tokens[l].type == DECNUM)
			sscanf(tokens[l].str, "%d", &num);
		else if (tokens[l].type == HEXNUM)
			sscanf(tokens[l].str, "%x", &num);
		else if (tokens[l].type == REGISTER) {
			if (strlen(tokens[l].str) == 3) { // 32-bit
				int i;
				//printf("%s\n", tokens[l].str);
				for (i = 0; i < 8; i++) { // check and find register name
					if (strcmp(tokens[l].str, regsl[i]) == 0)
						break;
				}
				if (i == 8 && strcmp(tokens[i].str, "eip") == 0)
					num = cpu.eip;
				else if (i == 8 && strcmp(tokens[i].str, "eip") != 0)
					Assert(1,"illegal register name.\n");
				else
					num = reg_l(i);
			}
			else if (strlen(tokens[l].str) == 2) { // 16-bit
				int i;
				for (i = 0; i < 8; i++) {
					if (strcmp(tokens[l].str, regsw[i]) == 0)
						break;
				}
				num = reg_w(i);
			}
			else if (strlen(tokens[l].str) == 1) { // 8-bit
				int i;
				for (i = 0; i < 8; i++) {
					if (strcmp(tokens[l].str, regsb[i]) == 0)
						break;
				}
				num = reg_b(i);
			}
			else Assert(2, "no register found.\n");
		}
		return num;
	}
	
	else if (check_parentheses(l, r) == true) {
		return eval(l+1, r-1);
	}
	
	else { // l < r
		int op = dominant_operator(l, r);
		printf("op=%d\n", op);
		if (tokens[op].type == '!' || l == op || tokens[op].type == NEGATIVE || tokens[op].type == DEREFERENCE) {
			uint32_t val0 = eval(l+1, r);
			switch (tokens[l].type) {
				case DEREFERENCE: return swaddr_read(val0, 4);
				case NEGATIVE: return -val0;
				case '!': return !val0;
				default: Assert(3, "default because no val0.\n");
			}
		}
		//printf("op=%d\n", op);
		uint32_t val1 = eval(l, op-1);
		uint32_t val2 = eval(op+1, r);
		printf("v1=%d, v2=%d\n", val1, val2);
		switch (tokens[op].type) {
			case '+': return val1 + val2;
			case '-': return val1 - val2;
			case '*': return val1 * val2;
			case '/': return val1 / val2;
			case EQ: return val1 == val2;
			case NEQ: return val1 != val2;
			case AND: return val1 && val2;
			case OR: return val1 || val2;
			default: break;
		}
	}

	return 0;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	int i;
	for (i = 0; i < nr_token; i++) {
		if (tokens[i].type == '*')
			if (i == 0 || (tokens[i-1].type != DECNUM && tokens[i-1].type != HEXNUM && tokens[i-1].type != REGISTER && tokens[i-1].type != ')')) {
				tokens[i].type = DEREFERENCE;
				tokens[i].priority = 6;
			}
		if (tokens[i].type == '-')
			if (i == 0 || (tokens[i-1].type != DECNUM && tokens[i-1].type != HEXNUM && tokens[i-1].type != REGISTER && tokens[i-1].type != ')')) {
				tokens[i].type = NEGATIVE;
				tokens[i].priority = 6;
			}

	}
	*success = true;
	printf("nr=%d\n", nr_token);
	return eval(0, nr_token-1);

	/* TODO: Insert codes to evaluate the expression. */
	//panic("please implement me");
	//return 0;
}

