/* File: hw4.c */
/* Solution by Chris Turgeon */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef enum { false, true } bool;




void shift_array(int shift_constants[], char *constant, char operation) {

	/* This function takes in an array by reference which is empty,
	 * a constant number in string form, and an operation character.
	 * Then, the function computes the shift left values if the 
	 * multiplication and the shift right value if division.
	 */

	// Convert to positive number if needed
	int num = atoi( constant );
	if ( num == 0 ) {
		shift_constants[0] = 0;
		return;
	}
	bool is_positive = false;
	if ( num == abs(num) ) {
		is_positive = true;
	} else {
		num = abs(num);
	}

	// Convert decimal constant to binary array
	if ( operation == '/' && abs(atoi(constant)) % 2 != 0 ) {
		shift_constants[0] = *constant;
		return;
	}
	int bit_array[32]; 
	int idx = 0;
	int i;
	for (i = 31; i >= 0; i--) {
		int k = num >> i;
		if (k & i)
			bit_array[idx] = 1;
		else 
			bit_array[idx] = 0;
		idx++;
	}

  // If multiplication by constant
	if ( operation == '*' ) { 
		// Get the constants used in shift instructions 
		idx = 0;
		for (i = 0; i < 32; i++) {
			if ( bit_array[i] == 1 ) {
				shift_constants[idx] = 31-i;
				idx++;
			}
		} // Last value is -1
		if (is_positive)
			shift_constants[idx] = -1;
		else 
			shift_constants[idx] = -2;
	}

	// If dividing by a constant
	else { 
		// Get the constant used in SRL instruction
		for (i = 0; i < 15; i++) {
			int comp_num = pow(2, i);
			if ( comp_num == num ) {
				shift_constants[0] = i;
				break;
			}
		} // Last value is -1
		if (is_positive)
			shift_constants[1] = -1;
		else 
			shift_constants[1] = -2;
	} 
}




void mult_by_const(char *left, int consts[], char *temp_regs[10], 
	                 int *t_trk, int *s_reg_cnt) {

	/* This function takes in the left register/value in multiplication,
	 * an array of constants used in the shifts, and the temporary register
	 * tracker by reference so that it can be incremented. 
   *
   * left is a $s or $t register depending, determined outside of func
	 */

	if ( consts[0] == 0 ) {
		printf("li $s%d,0\n", *s_reg_cnt);
		(*s_reg_cnt)++;
		return;
	}

	bool negative_constant = false;

	int idx;
	for (idx = 0; consts[idx] == abs(consts[idx]); idx++) {
		printf("sll %s,%s,%d\n",temp_regs[*t_trk],left,consts[idx]);
		if ( idx == 0 ) {
			printf("move %s,%s\n",temp_regs[*t_trk+1],temp_regs[*t_trk]);
		} else {
			printf("add %s,%s,%s\n",temp_regs[*t_trk+1],temp_regs[*t_trk+1],temp_regs[*t_trk]);
		}	
	}
	printf("add %s,%s,%s\n", temp_regs[*t_trk+1], temp_regs[*t_trk+1], left);
	char buf[4];
	buf[0] = '$';
	buf[1] = 's';
	buf[2] = *s_reg_cnt + '0';
	buf[3] = '\0';
	if (consts[idx] == -2) {
		negative_constant = true; 
	}
	if ( negative_constant ) {
		printf("sub %s,$zero,%s\n",buf,temp_regs[*t_trk+1]);
	} else {
		printf("move %s,%s\n",buf,temp_regs[*t_trk+1]);
	}
	(*t_trk)++;
	(*s_reg_cnt)++;
}




void div_by_const(char *left, int consts[], char *temp_regs[10],
	                int *t_trk, int *s_reg_cnt, int *curr_L, char *n) {

	/* This function takes in the left value of the operation, the
	 * SRL value, the array of temporary regs with the current for use,
	 * the current $s register for use and the current label value L
	 */

	bool negative_constant = false;
	if (abs(atoi(n)) % 2 != 0) {
		printf("li %s,%d\n", temp_regs[*t_trk], atoi(n));
		printf("div %s,%s\n", left, temp_regs[*t_trk]);
		printf("mflo $t%d\n", (*t_trk)+1);
		(*s_reg_cnt)++;
		(*curr_L)++;
		*t_trk = *t_trk + 1;
		return;
	}
	else if ( consts[1] == -2 )
		negative_constant = true;

	char buf[4];
	buf[0] = '$';
	buf[1] = 's';
	buf[2] = *s_reg_cnt + '0';
	buf[3] = '\0';
	int num = atoi(n);

	printf("bltz %s,L%d\n", left, *curr_L);
	printf("srl %s,%s,%d\n", buf, left, consts[0]);
	if ( negative_constant ) {
		printf("sub %s,$zero,%s\n", buf, buf);
	}
	printf("j L%d\n", (*curr_L)+1);
	printf("L%d:\n", *curr_L);
	printf("li %s,%d\n", temp_regs[*t_trk], num);
	printf("div %s,%s\n", left, temp_regs[*t_trk]);
	printf("mflo %s\n", buf);
	printf("L%d:\n", (*curr_L)+1);
	(*s_reg_cnt)++;
	*t_trk = *t_trk + 1;
	*curr_L = *curr_L + 2;  
}




int main(int argc, char *argv[]) {

	/* Open file stream and check to see if the file has been
	 * opened correctly; if not, print an error and return.
	 */

	FILE *file;
	if ( (file = fopen(argv[1], "r")) == NULL ) {
		perror("INVALID INPUT FILE: Try again!\n");
		return EXIT_FAILURE;
	}

	/* Assume the input size is no more than 128 characters. 
     * Read in all of the data by character and add to array.
	 */

	int s_reg_cnt = 0;   // saved register number
	int line_cnt = 0;    // keeps track of line
	int curr_line = 0;   // keeps track of current line num
	int label_count = 0; // keeps track of current labels

	char c_code;
	char *input = (char *) malloc( sizeof(char) * 128 );
	
	int i = 0;
	while ( fscanf(file, "%c", &c_code) != EOF ) {
			// Increment line count and retrieve line from input.
			if ( c_code == '\n' ) {
				line_cnt++;
			}
			input[i] = c_code;
			i++;
	}
	input[i] = '\0';
	char *print_trk = input;

	/* Terminate program if the input file is empty. */

	if ( i == 0 ) {
		perror("INVALID INPUT: File is empty!\n");
		return EXIT_FAILURE;
	}

	/* Assume there will be no more than 50 constants from input. 
	 * Assume constants will be no more than 8 digits in length.
	 * Loop through input, add constants into an array,
	 * and keep track of the total amount of constants.
	 */

	int constant_cnt = 0; // length of constant array
	int curr_const = 0;   // tracker of which constants have been used
	char **constants = (char **) malloc( sizeof(char *) * 50 ); 

	int j;
    for (j = 0; j < i; j++) {
		bool is_positive = isdigit(input[j]);
		bool is_negative = (input[j] == '-') && isdigit(input[j+1]);
		if ( is_positive || is_negative ) {
			char *constant = (char *) malloc( sizeof(char) * 8 );
			int k = 0;
			if ( is_negative ) { // Account for negative constant.
				constant[k] = '-';
				k++; j++;
			}
			while ( isdigit(input[j]) ) { // Load integer constant. 
			constant[k] = input[j];
			k++; j++;
			}
			constant[k] = '\0';
			constants[constant_cnt] = constant;
			constant_cnt++;
		}
	}

  /* Construct all of the needed temporary registers. 
   * Use temporary_tracker to keep track of the current
   * $t register needed for instructions.
   */

  int temp_tracker = 0;
  char *temporary_regs[10] = 
  {
  	"$t0\0", "$t1\0", "$t2\0", "$t3\0", "$t4\0", 
  	"$t5\0", "$t6\0", "$t7\0", "$t8\0", "$t9\0"
  };


  /* LOOP THROUGH INPUT AND DETERMINE PROPER INSTRUCTIONS */

  bool last_line = false;
  int s = 0;            // stores length of following 2 arrays
  char *saved_regs[20]; // array of $s registers for use later
  char c_variables[20]; // array of c variables assigned to consts

	j = 0;
	while ( j < i ) {

		if ( isalpha(input[j]) && (input[j+2] == '=') ) { // Found a LHS variable

			if ( !last_line ) {
				// Add to c_vars array
				c_variables[s] = input[j];
				// Create $s register and add to array.
				char *s_reg = (char *) calloc(4, sizeof(char));
				s_reg[0] = '$'; 
				s_reg[1] = 's';
				s_reg[2] = s_reg_cnt + '0';
				s_reg[3] = '\0';
				saved_regs[s] = s_reg;
				s++;
			}

			// Check for a positive or negative constant.
			bool is_positive = isdigit(input[j+4]);
			bool is_negative = (input[j+4] == '-') && isdigit(input[j+5]);
			if ( is_positive || is_negative ) { 
				// Print the line
				printf("# ");
				while (true) {
					printf("%c", *print_trk);
					if (*print_trk == '\n') break;
					print_trk++;
				}
				print_trk++;
				// Print and continue parsing.
				printf("li $s%d,%s\n", s_reg_cnt, constants[curr_const]); 
				s_reg_cnt++;
				curr_const++;
				curr_line++;
				// Advance to new line.
				while ( input[j] != '\n' ) { j++; }                       
				j += 1;                      
			}

			if ( !last_line ) {
				// Add to c_vars array
				c_variables[s] = input[j];
				// Create $s register and add to array.
				char *s_reg = (char *) calloc(4, sizeof(char));
				s_reg[0] = '$'; 
				s_reg[1] = 's';
				s_reg[2] = s_reg_cnt + '0';
				s_reg[3] = '\0';
				saved_regs[s] = s_reg;
				s++;
			}


			/* Loop through the given line of code to see how many operations
			 * are done. If more than one is done, then we need to save into a 
			 * $t register. Then find out how many operations are done in a line of 
			 * C code and assemble the $t registers that will be needed. Store these 
			 * $t registers in the 'temporary_regs' array and track the size
			 */

			int idx = j;
			int op_cnt = 0;
			while ( input[idx] != ';' ) {
				bool found_add = input[idx] == '+';
				bool found_sub = input[idx] == '-' && input[idx+1] == ' ';
				bool found_mul = input[idx] == '*';
				bool found_div = input[idx] == '/';
				bool found_mod = input[idx] == '%';
				if ( found_add || found_sub || found_mul || found_div || found_mod ) {
					op_cnt++;
				}
				idx++;
			}
		  bool multiple_operations = op_cnt > 1;
	

			/* Manages addition and subtraction between registers, including 
			 * instructions with constants involved. Loop through the C code,
			 * determine the operation, then utilize proper registers to 
			 * print MIPS instruction. 
			 */ 

			idx = j;
			int op_tracker = 1; // used to see if last op is reached
			bool done_with_first_op = false; 
			while ( (input[idx] != '\n') && (op_cnt != 0) ) {

				bool printed = false;
				while (*print_trk != '\0') {
					if (!printed) {
						printf("# ");
						printed = true;
					}
					printf("%c", *print_trk);
					if (*print_trk == '\n') break;
					print_trk++;
				}	
				print_trk++;

				// Booleans for constant mult or div
				bool mult_by_constant = false;
				bool div_by_constant = false;
				bool found_mod = false;
				int shift_constants[10];

				// Get to operation within a line of C code and print
				while ( true ) {
					if ( input[idx+1] == '+' ) { 
						printf("add ");
						idx++;
						break;
					}

					else if ( input[idx+1] == '-' && input[idx+2] == ' ' ) { 
						printf("sub "); 
						idx++;
						break;
					}

					else if ( input[idx+1] == '*' ) {
						idx++;
		
						// Regular Case: multiply by registers or non 1 or 0 constant
						if ( isdigit(input[idx+2]) || (input[idx+2] == '-') ) {
							shift_array(shift_constants, constants[curr_const], input[idx]);
							mult_by_constant = true;
						} else {
							printf("mult ");	
						}
						break;
					} 

					else if ( input[idx+1] == '/' ) {
						idx++;

						// Regular Case: divide by registers or non 1 or 0 constant
						if ( isdigit(input[idx+2]) || (input[idx+2] == '-')  ) {
							shift_array(shift_constants, constants[curr_const], input[idx]);
							div_by_constant = true;
						} else {
							printf("div ");
						}
						break;
					}
					else if ( input[idx+1] == '%' ) {
						idx++;
						found_mod = true;
						break;
					}
					idx++; 
				}

				// Identify the left and right operands.
				char left_var = input[idx-2];
				char right_var = input[idx+2];
				char buf_left[4];

				// Find the proper $s or $t register that holds the left_var.
				int k;
				if ( done_with_first_op ) {
					strcpy(buf_left, temporary_regs[temp_tracker]);
				} else {
					bool found = false;
					for (k = 0; k < s; k++) {
						if ( left_var == c_variables[k] ) {
							strcpy(buf_left, saved_regs[k]);
							found = true;
						}
						if ( !found ) {
							strcpy(buf_left, temporary_regs[temp_tracker]);
						}
					}
				}

	
				/* Check to see if right-most element in MIPS instruction is 
				 * a constand or a $s register. Also determine if MIPS instruction
				 * should be printed in add/sub form or mult/div form.
				 * (i.e.) add $t,$s,$s   versus   mult $s,$s
				 */

				bool const_is_one = false;
				bool const_is_neg_one = false;
				char buf_right[4];

				if ( right_var == '-' || isdigit(right_var) ) {
					if ( atoi(constants[curr_const]) == 1 ) 
						const_is_one = true;
					else if ( atoi(constants[curr_const]) == -1 ) 
						const_is_neg_one = true;		
					strcpy(buf_right, constants[curr_const]); 
					curr_const++;
				} else { 
					for (k = 0; k < s; k++) {
						if ( right_var == c_variables[k] )
							strcpy(buf_right, saved_regs[k]); 
					}
					done_with_first_op = true; 
				}


				/* Determine the type of register, $s or $t, that will be printed 
				 * and will hold the result of the operation performed. Also determine 
				 * the register number.
				 */

				char reg_type[4];

				// Determine if currently dealing with mult or div
				bool mult_or_div = input[idx] == '*' || input[idx] == '/'; 
				if ( mult_or_div && !const_is_one && !const_is_neg_one ) {
					reg_type[0] = '\0';
				} 

				// Establish register to hold result of add or sub
				else {
					int reg_cnt;
					if ( op_cnt == 1 || op_tracker == op_cnt ) {
						reg_cnt = s_reg_cnt;
						reg_type[0] = '$';
						reg_type[1] = 's';
						reg_type[2] = reg_cnt + '0';
						reg_type[3] = '\0';
					}
					else {
						strcpy(reg_type, temporary_regs[temp_tracker]);
						temp_tracker++;
					} 
				}


// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //


				// Multiplication by constant, check for 1 or -1 or 0
				if ( mult_or_div ) {
					if ( mult_by_constant ) {
						// Set up $s register one greater than reg_type
						char s_reg_buf[4];
						strcpy(s_reg_buf, reg_type);
						s_reg_buf[2] = s_reg_cnt + '0';
						if ( const_is_one ) {
							printf("move %s,%s\n", reg_type, buf_left);
							printf("move %s,%s\n", s_reg_buf, reg_type);
							s_reg_cnt++;
						}
						else if ( const_is_neg_one ) {
							printf("move %s,%s\n", reg_type, buf_left);
							printf("sub %s,$zero,%s\n", s_reg_buf, reg_type);
							s_reg_cnt++;
						} else {
								mult_by_const(buf_left, shift_constants, temporary_regs,
						                  &temp_tracker, &s_reg_cnt);	
						}
					}

					// Division by constant, check for 1 or -1 or 0
					else if ( div_by_constant ) {
						char s_reg_buf[4];
						strcpy(s_reg_buf, reg_type);
						s_reg_buf[2] = s_reg_cnt + '0';
						if ( const_is_one ) {
							printf("move %s,%s\n", s_reg_buf, buf_left);
							s_reg_cnt++;
						}
						else if ( const_is_neg_one ) {
							printf("sub %s,$zero,%s\n", s_reg_buf, buf_left);
							s_reg_cnt++;
						} else {
								div_by_const(buf_left, shift_constants, temporary_regs,
							    					 &temp_tracker, &s_reg_cnt, &label_count, 
									    			 buf_right);
						}
					} 

					else {
						printf("%s%s,%s\n", reg_type, buf_left, buf_right);
						// 'mflo' stored in temporary register.
						if ( (multiple_operations && (op_tracker != op_cnt)) || !last_line ) { 
							printf("mflo %s\n", temporary_regs[temp_tracker]); 
						}
						// 'mflo' stored in saved register.
						else if ( op_tracker == op_cnt ) {
							s_reg_cnt++;
							printf("mflo $s%d\n", s_reg_cnt);
						} 
					}
				}

				// If a mod sign is found
				else if ( found_mod ) {
					printf("div %s,%s\n", buf_left, buf_right);
					if ( line_cnt ) {
						printf("mfhi $s%d\n", s_reg_cnt);
						s_reg_cnt++;
					}
					else {
						printf("mfhi %s\n", temporary_regs[temp_tracker]);
						temp_tracker++;
					}
				} 

				// If addition or subtraction.
				else {
					printf("%s,%s,%s\n", reg_type, buf_left, buf_right);
				}
				op_tracker++;


				/* Reach the end of the current line of C code if only one operation
				 * or the last operation has just been processed.
				 * If more than one operation, loop and find next operation.
				 */

				curr_line++;
				if ( curr_line == line_cnt ) 
					last_line = true;

				// Single operation found.
				if ( !multiple_operations ) {
					while ( input[idx] != '\n' ) { idx++; }
					j = idx + 1;
				}
				// Last operation reached in non-last line.
				else if ( op_cnt+1 == op_tracker && !last_line ) { 
					while ( input[idx] != '\n' ) { idx++; }
					j = idx + 1;
				} 
				// Last line reached and non-last op.
				if ( last_line && op_tracker == op_cnt+1 ) {  
					while ( input[idx] != '\n' ) { idx++; }
					j = idx + 1;
				} 
			}
		}
	}


	/* Free all of the dynamically allocated memory,
	 * close the file and return to the user.
	 */

	int p;
	for (p = 0; p < constant_cnt; p++) {
		free(constants[p]);
	}
	for (p = 0; p < s; p++) {
		free(saved_regs[p]);
	}
	free(constants);
	free(input);

	fclose(file);
	return EXIT_SUCCESS;
}