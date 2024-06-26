/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

const char *reg_name[] = {
  "$0", "$ra", "$sp", "$gp", "$tp", "$t0", "$t1", "$t2",
  "$s0", "$s1", "$a0", "$a1", "$a2", "$a3", "$a4", "$a5",
  "$a6", "$a7", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$s8", "$s9", "$s10", "$s11", "$t3", "$t4", "$t5", "$t6"
};

enum {
  TK_NOTYPE = 256, //TK_EQ,
  NUM = 1,
  RESGISTER = 2,
  HEX = 3,
  EQ = 4,
  NOTEQ = 5,
  OR = 6,
  AND = 7,
  ZUO = 8,
  YOU = 9,
  LEQ = 10,
  YINYONG = 11,
  POINT, NEG
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  // {"\\+", '+'},         // plus
  // {"==", TK_EQ},        // equal
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // sub
  {"\\*", '*'},         // mul
  {"\\/", '/'},         // div

  {"\\(", ZUO},
  {"\\)", YOU},
  /*
    * Inset the '(' and ')' on the [0-9] bottom case Bug.
    */
  {"\\<\\=", LEQ},		
  {"\\=\\=", EQ},        // equal
  {"\\!\\=", NOTEQ},

  {"\\|\\|", OR},       // Opetor
  {"\\&\\&", AND},
  {"\\!", '!'},

  //{"\\$[a-z]*", RESGISTER},
  {"\\$[a-zA-Z]*[0-9]*", RESGISTER},
  {"0[xX][0-9a-fA-F]+", HEX},
  {"[0-9]*", NUM},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};
bool division_zero = false;
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[100];
} Token;

static Token tokens[100] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;
int len = 0;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        //printf("e[position] = %s\n", e);
        for (i = 0; i < NR_REGEX; i ++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                //char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;
                //printf("substr_len = %d\n", substr_len);
                /*
                   Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                   i, rules[i].regex, position, substr_len, substr_len, substr_start);
                   */

                position += substr_len;
                //printf("tokens_type = %d\n", rules[i].token_type);
                
                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */
                Token tmp_token;
                switch (rules[i].token_type) {
                    case '+':
                        tmp_token.type = '+';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case '-':
                        tmp_token.type = '-';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case '*':
                        tmp_token.type = '*';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case '/':
                        tmp_token.type = '/';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case 256:
                        break;
                    case '!':
                        tmp_token.type = '!';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case 9:
                        tmp_token.type = ')';
                        tokens[nr_token ++] = tmp_token;
                        break;
                    case 8:
                        tmp_token.type = '(';
                        tokens[nr_token ++] = tmp_token;
                        break;

                        // Special
                    case 1: // num
                        tokens[nr_token].type = 1;
                        strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
                        nr_token ++;
                        break;
                    case 2: // regex
                        tokens[nr_token].type = 2;
                        //printf("%s",&e[position - substr_len]);
                        strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
                        //printf("%s",tokens[0].str);
                        nr_token ++;
                        break;
                    case 3: // HEX
                        tokens[nr_token].type = 3;
                        strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
                        nr_token ++;
                        break;
                    case 4:
                        tokens[nr_token].type = 4;
                        strcpy(tokens[nr_token].str, "==");
                        nr_token++;
                        break;
                    case 5:
                        tokens[nr_token].type = 5;
                        strcpy(tokens[nr_token].str, "!=");
                        nr_token++;case 6:
                        tokens[nr_token].type = 6;
                        strcpy(tokens[nr_token].str, "||");
                        nr_token++;
                        break;
                    case 7:
                        tokens[nr_token].type = 7;
                        strcpy(tokens[nr_token].str, "&&");
                        nr_token++;
                        break;
                    case 10:
                        tokens[nr_token].type = 10;
                        strcpy(tokens[nr_token].str, "<=");
                        nr_token ++;
                        break;
                    default:
                        printf("i = %d and No rules is com.\n", i);
                        break;
                }
                len = nr_token;
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}



void int2char(int x, char str[]){
    int len = strlen(str);
    memset(str, 0, len);
    int tmp_index = 0;
    int tmp_x = x;
    int x_size = 0, flag = 1;
    while(tmp_x){
      tmp_x /= 10;
      x_size ++;
      flag *= 10;
    }
    flag /= 10;
    while(x)
    {
      int a = x / flag; 
      x %= flag;
      flag /= 10;
      str[tmp_index ++] = a + '0';
    }
}

int char2int(char s[]){
    int s_size = strlen(s);
    int res = 0 ;
    for(int i = 0 ; i < s_size ; i ++)
    {
	res += s[i] - '0';
	res *= 10;
    }
    res /= 10;
    return res;
}

bool check_parentheses(int p, int q)
{
    if(tokens[p].type != '('  || tokens[q].type != ')')
	  return false;

    int l = p , r = q;
    while(l < r)
    {
      if(tokens[l].type == '('){
        if(tokens[r].type == ')')
        {
          l ++ , r --;
          continue;
        } 
        else{
          r --;
          if (l == r) return false;
        }
      }
      // else if(tokens[l].type == ')')
      //   return false;
      else{
        if(tokens[l].type == ')') return false;
        l++;
        if(tokens[l].type == ')') return false;//前后都要检测一遍
      }
    }
    return true;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

uint32_t eval(int p, int q) {
    if (p > q) {
        /* Bad expression */
        //assert(0 && "Bad expression");
        return -1;
    }
    else if (p == q) {
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */
        if (tokens[p].type == 2){
          // printf("%d",p);
          // printf("%s", tokens[p].str);
          //printf("%d", tokens[p].type);
          // puts(tokens[p].str);
          for (int i = 0; i < 32; i++){
            //assert(0);
            if (strcmp(tokens[p].str, reg_name[i]) == 0){
              //assert(0);
              return cpu.gpr[i];
            }
          }
        }
        return atoi(tokens[p].str);
    }
    else if (check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        // printf("check p = %d, q = %d\n",p + 1 , q - 1);
        return eval(p + 1, q - 1);
    }
    // else if(check_parentheses(p, q) == false){
    //   assert(0);
    //   return -1;
    // }
    else {
        int op = -1; // op = the position of 主运算符 in the token expression;
        bool flag = false;
        for(int i = p ; i <= q ; i ++)
        {
          // printf("%d", i);
            if(tokens[i].type == '(')
            {
                int j = i;
                i++;
                while(check_parentheses(j, i) == false)
                  i++;
            }
            if(!flag && tokens[i].type == 6){
                flag = true;
                op = max(op,i);
            }

            if(!flag && tokens[i].type == 7 ){
				        flag = true;
                op = max(op,i);
            }

            if(!flag && tokens[i].type == 5){
                flag = true;
                op = max(op,i);
            }

            if(!flag && tokens[i].type == 4){
                flag = true;
                op = max(op,i);
            }
            if(!flag && tokens[i].type == 10){
                flag = true;
                op = max(op, i);
            }
            if(!flag && (tokens[i].type == '+' || tokens[i].type == '-')){
                flag = true;
                op = max(op, i);
            }
            if(!flag && (tokens[i].type == '*' || tokens[i].type == '/') ){
                op = max(op, i);
            }
        }
        // printf("op position is %d\n", op);
        // if register return $register
        int  op_type = tokens[op].type;

        // 递归处理剩余的部分
        uint32_t  val1 = eval(p, op - 1);
        uint32_t  val2 = eval(op + 1, q);
        //printf("val1 = %d, val2 = %d \n", val1, val2);

        switch (op_type) {
            case '+':
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '*':
                return val1 * val2;
            case '/':
                if(val2 == 0){//printf("division can't zero;\n");
                    division_zero = true;
                    return 0;
                }
                return val1 / val2;
            case 4:
                return val1 == val2;
            case 5:
                return val1 != val2;
            case 6:
                return val1 || val2;
            case 7:
                return val1 && val2;
            default:
                printf("No Op type.");
                assert(0);
        }
    }
}


word_t expr(char *e, bool *success)
{
  if (!make_token(e)) {
    assert(0 && "make_token error");
    *success = false;
    return 0;
  }

    /* TODO: Insert codes to evaluate the expression. */
    *success = true;

    /*
     *  Get length
     */ 
    int tokens_len = 0;
    for(int i = 0 ; i < 100 ; i ++)
    {
      if(tokens[i].type != 0)
      tokens_len ++;
    }
    // int tokens_len_F = tokens_len;

    /*
     * Init the tokens regex
     */
    // for(int i = 0 ; i < tokens_len ; i ++)
    // {
    //   if(tokens[i].type == 2)
    //   {
		// bool flag = true;
		// int tmp = isa_reg_str2val(tokens[i].str, &flag);
		// if(flag){
		// 	int2char(tmp, tokens[i].str); // transfrom the str --> $egx
		// }
		// else{
		// 	printf("Transfrom error. \n");
		// 	assert(0);
	  //   }
    //   } 
    // }


    /*
     * Init the tokens HEX
     */
    for(int i = 0 ; i < tokens_len ; i ++)
    {
      if(tokens[i].type == 3)// Hex num
      {
        int value = strtol(tokens[i].str, NULL, 16);
        int2char(value, tokens[i].str);
      }
    }

    /*
     * Init the tokens str. 1 ==> -1.
     */
    for(int i = 0 ; i < tokens_len ; i ++)
    {
      if((tokens[i].type == '-' && i > 0 && tokens[i-1].type != NUM && tokens[i-1].type != ')' && tokens[i+1].type == NUM)||(tokens[i].type == '-' && i == 0))
      {
        tokens[i].type = TK_NOTYPE;
        //tokens[i].str = tmp;
        //从上到下才不会覆盖
        for(int j = 31 ; j > 0 ; j --){
          tokens[i+1].str[j] = tokens[i+1].str[j-1];
        }
        tokens[i+1].str[0] = '-';
        // printf("%s\n", tokens[i+1].str);
        for(int j = 0 ; j < tokens_len ; j ++){
          if(tokens[j].type == TK_NOTYPE)
          {
            for(int k = j + 1 ; k < tokens_len ; k ++){
              tokens[k - 1] = tokens[k];
            }
            tokens_len -- ;
          }
        }
      }
    }

    /*
     * Init the tokens !
     */
    for(int i = 0 ; i < tokens_len ; i ++)
    {
		if(tokens[i].type == '!')
		{
			tokens[i].type = TK_NOTYPE;
			int tmp = char2int(tokens[i+1].str);
			if(tmp == 0){
				memset(tokens[i+1].str, 0 ,sizeof(tokens[i+1].str));
				tokens[i+1].str[0] = '1';
			}
			else{
				memset(tokens[i+1].str, 0 , sizeof(tokens[i+1].str));
			}
			for(int j = 0 ; j < tokens_len ; j ++){
				if(tokens[j].type == TK_NOTYPE)
				{
					for(int k = j +1 ; k < tokens_len ; k ++){
					tokens[k - 1] = tokens[k];
					}
					tokens_len -- ;
				}
			}
		}
    }
    for(int i = 0 ; i < tokens_len ; i ++)
    {
      if(	(tokens[i].type == '*' && i > 0 && tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != RESGISTER && tokens[i-1].type != ')' && tokens[i+1].type == NUM )
        || (tokens[i].type == '*' && i > 0 && tokens[i-1].type != NUM && tokens[i-1].type != HEX && tokens[i-1].type != RESGISTER && tokens[i-1].type != ')' && tokens[i+1].type == HEX)
        || (tokens[i].type == '*' && i == 0))
      {
        tokens[i].type = TK_NOTYPE;
        int tmp = char2int(tokens[i+1].str);
        uintptr_t a = (uintptr_t)tmp;
        int value = *((int*)a);
        int2char(value, tokens[i+1].str);	    
        for(int j = 0 ; j < tokens_len ; j ++){
          if(tokens[j].type == TK_NOTYPE)
          {
            for(int k = j +1 ; k < tokens_len ; k ++){
              tokens[k - 1] = tokens[k];
            }
            tokens_len -- ;
          }
        }
      }
    }

    /*
     * True Expr
     * Data tokens (num, op_type)
     */ 
    uint32_t res = 0;
    //printf("tokens_len = %d\n", tokens_len);
    //printf("%s",tokens[0].str);
    res = eval(0, tokens_len - 1);
    //printf("check flag = %d\n",check_parentheses(0, tokens_len - 1));
    //if(!division_zero)
		//printf("uint32_t res = %d\n", res);
    if(division_zero)
		printf("Your input have an error: can't division zeor\n");    
    memset(tokens, 0, sizeof(tokens));//计算完毕归零
    return res;
}

// int main()
// {
// 	char e[500] = "((((48/12)-18)/2)*(27)+29-(((99-1))+58)/22+48/72-((83)))";
// 	bool success;
// 	u_int32_t res = expr(e, &success);
// 	unsigned result = ((((48/12)-18)/2)*(27)+29-(((99-1))+58)/22+48/72-((83)));
// 	printf("res = %d\n", res);
// 	printf("result = %d\n", result);
// }
