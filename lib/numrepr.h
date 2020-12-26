#ifndef numrepr_h

/* begin numrepr.h */
#define numrepr_h

/* 
   types and constants to assist with numeric representations

   the main features are an enum type representing the numeric value
   of all valid digits, an enum type representing the numeric value
   of all valid bases (2, 8, 10, and 16), a constant representing all
   valid numeric prefixes, and a constant representing valid numeric prefixes.

 */

typedef enum {
  DIGIT_INVALID = -1,
  DIGIT_0,
  DIGIT_1,
  DIGIT_2,
  DIGIT_3,
  DIGIT_4,
  DIGIT_5,
  DIGIT_6,
  DIGIT_7,
  DIGIT_8,
  DIGIT_9,
  DIGIT_A,
  DIGIT_B,
  DIGIT_C,
  DIGIT_D,
  DIGIT_E,
  DIGIT_F,
} digit_t;

const int num_digits = 16;
const int num_prefixes = 11;

const int numchr_table[16] =  { '0', '1', '2', '3',
                                '4', '5', '6', '7',
                                '8', '9', 'a', 'b',
                                'c', 'd', 'e', 'f',
};

enum {
  PREFIX_NEG,
  PREFIX_POS,
  PREFIX_BIN,
  PREFIX_NBIN,
  PREFIX_PBIN,
  PREFIX_OCT,
  PREFIX_NOCT,
  PREFIX_POCT,
  PREFIX_HEX,
  PREFIX_NHEX,
  PREFIX_PHEX,
  NUM_PREFIXES,
};

const char* valid_prefixes[] = {"-", "+",
                                "0b", "-0b", "+0b",
				"0o", "-0o", "+0o",
				"0x", "-0x", "+0x",
};

const int prefix_lengths[] = { 1, 1,
                               2, 3, 3,
                               2, 3, 3,
                               2, 3, 3,
};

typedef enum {
  NBASE_INVALID   = -1,
  NBASE_2         =  2,
  NBASE_8         =  8,
  NBASE_10        = 10,
  NBASE_16        = 16,
} num_base_t;

/* end numrepr.h */
#endif
