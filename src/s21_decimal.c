#include "s21_decimal.h"

int get_bit(const s21_decimal value, int bit) {
  int result = 0;
  if (bit / 32 < 4) {
    unsigned int mask = 1u << (bit % 32);
    result = value.bits[bit / 32] & mask;
  }
  return !!result;
}

void set_bit(s21_decimal *varPtr, int bit, int value) {
  unsigned int mask = 1u << (bit % 32);
  if (bit / 32 < 4 && value) {
    varPtr->bits[bit / 32] |= mask;
  } else if (bit / 32 < 4 && !value) {
    varPtr->bits[bit / 32] &= ~mask;
  }
}

void set_sign(s21_decimal *value, int sign) {
  unsigned int mask = 1u << 31;
  if (sign != 0)
    value->bits[3] |= mask;
  else
    value->bits[3] &= ~mask;
}

int get_sign(const s21_decimal *value) {
  unsigned int mask = 1u << 31;
  return !!(value->bits[3] & mask);
}

int get_scale(const s21_decimal *value) { return (char)(value->bits[3] >> 16); }

void set_scale(s21_decimal *value, int scale) {
  int clearMask = ~(0xFF << 16);
  value->bits[3] &= clearMask;
  int mask = scale << 16;
  value->bits[3] |= mask;
}

int last_bit(s21_decimal number) {
  int last_bit = 95;
  while (last_bit >= 0 && get_bit(number, last_bit) == 0) {
    last_bit--;
  }
  return last_bit;
}

int shift_left(s21_decimal *value, int offset) {
  int res = OK;
  int lastbit = last_bit(*value);
  if (lastbit + offset > 95) {
    res = INF;
  } else {
    for (int i = 0; i < offset; i++) {
      int bit31 = get_bit(*value, 31);
      int bit63 = get_bit(*value, 63);
      value->bits[0] <<= 1;
      value->bits[1] <<= 1;
      value->bits[2] <<= 1;
      if (bit31) set_bit(value, 32, 1);
      if (bit63) set_bit(value, 64, 1);
    }
  }
  return res;
}

int is_zero(s21_decimal val1, s21_decimal val2) {
  int is_zero = FALSE;
  s21_decimal *ptr1 = &val1;
  s21_decimal *ptr2 = &val2;
  if (ptr1 && ptr2) {
    if (!val1.bits[0] && !val2.bits[0] && !val1.bits[1] && !val2.bits[1] &&
        !val1.bits[2] && !val2.bits[2]) {
      is_zero = TRUE;
    }
  }
  return is_zero;
}

void clear_bits(s21_decimal *val) { memset(val->bits, 0, sizeof(val->bits)); }

void bits_copy(s21_decimal src, s21_decimal *dest) {
  dest->bits[0] = src.bits[0];
  dest->bits[1] = src.bits[1];
  dest->bits[2] = src.bits[2];
  dest->bits[3] = src.bits[3];
}

int bit_addition(s21_decimal value1, s21_decimal value2, s21_decimal *res) {
  clear_bits(res);
  int return_val = OK;
  int buffer = 0;

  for (int i = 0; i < 96; i++) {
    int current_bit1 = get_bit(value1, i);
    int current_bit2 = get_bit(value2, i);

    if (!current_bit1 && !current_bit2) {
      if (buffer) {
        set_bit(res, i, 1);
        buffer = 0;
      } else {
        set_bit(res, i, 0);
      }
    } else if (current_bit1 != current_bit2) {
      if (buffer) {
        set_bit(res, i, 0);
        buffer = 1;
      } else {
        set_bit(res, i, 1);
      }
    } else {
      if (buffer) {
        set_bit(res, i, 1);
        buffer = 1;
      } else {
        set_bit(res, i, 0);
        buffer = 1;
      }
    }
    if (i == 95 && buffer == 1) return_val = INF;
  }
  return return_val;
}

int s21_add(s21_decimal number_1, s21_decimal number_2, s21_decimal *result) {
  clear_bits(result);
  int return_value = OK;

  if (!get_sign(&number_1) && !get_sign(&number_2)) {
    if (get_scale(&number_1) != get_scale(&number_2)) {
      scale_equalize(&number_1, &number_2);
    }

    int bit_additioin_result = OK;
    s21_decimal tmp_res;
    bit_additioin_result = bit_addition(number_1, number_2, &tmp_res);

    if (bit_additioin_result == INF) {
      return_value = INF;
    } else {
      *result = tmp_res;
      result->bits[3] = number_1.bits[3];
    }

  } else if (get_sign(&number_1) && !get_sign(&number_2)) {
    set_sign(&number_1, 0);
    return_value = s21_sub(number_2, number_1, result);

  } else if (!get_sign(&number_1) && get_sign(&number_2)) {
    set_sign(&number_2, 0);
    return_value = s21_sub(number_1, number_2, result);

  } else {
    set_sign(&number_1, 0);
    set_sign(&number_2, 0);
    return_value = s21_add(number_1, number_2, result);
    if (return_value == INF)
      return_value = NEGATIVE_INF;
    else
      set_sign(result, 1);
  }

  return return_value;
}

int s21_sub(s21_decimal number_1, s21_decimal number_2, s21_decimal *result) {
  clear_bits(result);
  int return_value = OK;

  int sign_1 = get_sign(&number_1);
  int sign_2 = get_sign(&number_2);
  if (get_scale(&number_1) != get_scale(&number_2)) {
    scale_equalize(&number_1, &number_2);
  }
  set_sign(&number_1, sign_1);
  set_sign(&number_2, sign_2);

  int resultSign;

  if (get_sign(&number_1) != get_sign(&number_2)) {
    resultSign = get_sign(&number_1);
    set_sign(&number_1, 0);
    set_sign(&number_2, 0);
    return_value = s21_add(number_1, number_2, result);
    if (return_value == INF)
      return_value = NEGATIVE_INF;
    else
      set_sign(result, resultSign);

  } else {
    if (s21_is_equal(number_1, number_2) == TRUE) {
    } else {
      int sign1 = get_sign(&number_1);
      int sign2 = get_sign(&number_2);
      set_sign(&number_1, 0);
      set_sign(&number_2, 0);
      s21_decimal *smallPtr, *bigPtr;

      if (s21_is_less(number_1, number_2) == TRUE) {
        smallPtr = &number_1;
        bigPtr = &number_2;
        resultSign = !sign2;
      } else {
        smallPtr = &number_2;
        bigPtr = &number_1;
        resultSign = sign1;
      }

      bit_subtraction(*bigPtr, *smallPtr, result);
      set_scale(result, get_scale(&number_1));
      set_sign(result, resultSign);
    }
  }

  return return_value;
}

int s21_mul(s21_decimal number_1, s21_decimal number_2, s21_decimal *result) {
  clear_bits(result);
  int error_code = OK;
  int res_sign;

  if (get_sign(&number_1) != get_sign(&number_2)) {
    res_sign = 1;
  } else {
    res_sign = 0;
  }
  int last_bit1 = last_bit(number_1);
  s21_decimal tmp = {{0, 0, 0, 0}};
  int inf_check = OK;

  for (int i = 0; i <= last_bit1; i++) {
    clear_bits(&tmp);
    int currbit = get_bit(number_1, i);

    if (currbit) {
      tmp = number_2;
      inf_check = shift_left(&tmp, i);
      inf_check = bit_addition(*result, tmp, result);
    }
  }
  if (inf_check == INF) {
    if (res_sign)
      error_code = NEGATIVE_INF;
    else
      error_code = INF;
    clear_bits(result);
  } else {
    int scale = get_scale(&number_1) + get_scale(&number_2);
    set_scale(result, scale);
    set_sign(result, res_sign);
  }
  return error_code;
}

int s21_div(s21_decimal divident, s21_decimal divisor, s21_decimal *result) {
  int return_value = OK;
  clear_bits(result);

  s21_decimal zero = {{0, 0, 0, 0}};

  if (s21_is_equal(divisor, zero) == TRUE) {
    return_value = DIVISION_BY_ZERO;
    clear_bits(result);

  } else {
    int beginScale = get_scale(&divident) - get_scale(&divisor);
    int resultSign = get_sign(&divident) != get_sign(&divisor);

    s21_decimal remainder = {0}, tmp = {0};

    set_scale(&divisor, 0);
    set_scale(&divident, 0);
    set_sign(&divisor, 0);
    set_sign(&divident, 0);

    bit_division(divident, divisor, &remainder, &tmp);
    bits_copy(tmp, result);

    s21_decimal border_value = {{4294967295u, 4294967295, 4294967295u, 0}};
    s21_decimal ten = {{10, 0, 0, 0}};
    set_scale(&border_value, 1);

    int inside_scale = 0;

    for (; inside_scale <= 27 && s21_is_equal(remainder, zero) == FALSE;) {
      if (s21_is_less(*result, border_value) == FALSE) {
        break;
      }
      s21_mul(remainder, ten, &remainder);
      bit_division(remainder, divisor, &remainder, &tmp);
      s21_mul(*result, ten, result);
      bit_addition(*result, tmp, result);
      inside_scale++;
    }

    s21_decimal musor;
    int endScale = beginScale + inside_scale;
    for (; endScale > 28;) {
      bit_division(*result, ten, &musor, result);
      endScale--;
    }
    for (; endScale < 0;) {
      s21_mul(*result, ten, result);
      endScale++;
    }

    set_scale(result, endScale);
    set_sign(result, resultSign);
  }

  return return_value;
}

int s21_mod(s21_decimal number_1, s21_decimal number_2, s21_decimal *result) {
  clear_bits(result);
  int return_value = OK;
  s21_decimal zero = {{0, 0, 0, 0}};
  if (s21_is_equal(number_2, zero)) {
    return_value = DIVISION_BY_ZERO;
  } else {
    if (!get_sign(&number_1) && !get_sign(&number_2)) {
      while (s21_is_greater_or_equal(number_1, number_2)) {
        s21_sub(number_1, number_2, &number_1);
      }
    } else if (!get_sign(&number_1) && get_sign(&number_2)) {
      set_sign(&number_2, 0);
      while (s21_is_greater_or_equal(number_1, number_2)) {
        s21_sub(number_1, number_2, &number_1);
      }
    } else if (get_sign(&number_1) && !get_sign(&number_2)) {
      set_sign(&number_1, 0);
      while (s21_is_greater_or_equal(number_1, number_2)) {
        s21_sub(number_1, number_2, &number_1);
      }
      set_sign(&number_1, 1);
    } else if (get_sign(&number_1) && get_sign(&number_2)) {
      set_sign(&number_1, 0);
      set_sign(&number_2, 0);
      while (s21_is_greater_or_equal(number_1, number_2)) {
        s21_sub(number_1, number_2, &number_1);
      }
      set_sign(&number_1, 1);
    }
    *result = number_1;
  }

  return return_value;
}

void bit_subtraction(s21_decimal number_1, s21_decimal number_2,
                     s21_decimal *result) {
  clear_bits(result);
  if (s21_is_not_equal(number_1, number_2) == TRUE) {
    int lastbit_num1 = last_bit(number_1);
    int mem = 0;
    for (int i = 0; i <= lastbit_num1; i++) {
      int currbit_num1 = get_bit(number_1, i);
      int currbit_num2 = get_bit(number_2, i);
      if (!currbit_num1 && !currbit_num2) {
        if (mem) {
          mem = 1;
          set_bit(result, i, 1);
        } else {
          set_bit(result, i, 0);
        }
      } else if (currbit_num1 && !currbit_num2) {
        if (mem) {
          mem = 0;
          set_bit(result, i, 0);
        } else {
          set_bit(result, i, 1);
        }
      } else if (!currbit_num1 && currbit_num2) {
        if (mem) {
          mem = 1;
          set_bit(result, i, 0);
        } else {
          mem = 1;
          set_bit(result, i, 1);
        }
      } else if (currbit_num1 && currbit_num2) {
        if (mem) {
          mem = 1;
          set_bit(result, i, 1);
        } else {
          set_bit(result, i, 0);
        }
      }
    }
  }
}

int s21_is_greater(s21_decimal number_1, s21_decimal number_2) {
  int sign1 = get_sign(&number_1);
  int sign2 = get_sign(&number_2);
  if (get_scale(&number_1) != get_scale(&number_2)) {
    scale_equalize(&number_1, &number_2);
  }
  set_sign(&number_1, sign1);
  set_sign(&number_2, sign2);
  int result = 1;

  if (get_sign(&number_1) && !get_sign(&number_2)) {
    result = 0;
  } else if (!get_sign(&number_1) && get_sign(&number_2)) {
    result = 1;
  } else if (!get_sign(&number_1) && !get_sign(&number_2)) {
    for (int i = 2; i >= 0; i--) {
      if (number_1.bits[i] > number_2.bits[i]) {
        result = TRUE;
        break;
      } else if (number_1.bits[i] < number_2.bits[i]) {
        result = FALSE;
        break;
      } else if (number_1.bits[i] == number_2.bits[i]) {
        result = FALSE;
        continue;
      }
    }
  } else if (get_sign(&number_1) && get_sign(&number_2)) {
    for (int i = 2; i >= 0; i--) {
      if (number_1.bits[i] > number_2.bits[i]) {
        result = 0;
        break;
      } else if (number_1.bits[i] < number_2.bits[i]) {
        result = 1;
        break;
      } else if (number_1.bits[i] == number_2.bits[i]) {
        result = 0;
        continue;
      }
    }
  }
  if (is_zero(number_1, number_2)) result = FALSE;
  return result;
}

int s21_is_equal(s21_decimal value_1, s21_decimal value_2) {
  int is_equal = TRUE;

  if (get_scale(&value_1) != get_scale(&value_2))
    scale_equalize(&value_1, &value_2);

  if (value_1.bits[0] == 0 && value_2.bits[0] == 0 && value_1.bits[1] == 0 &&
      value_2.bits[1] == 0 && value_1.bits[2] == 0 && value_2.bits[2] == 0) {
  } else if (value_1.bits[0] != value_2.bits[0] ||
             value_1.bits[1] != value_2.bits[1] ||
             value_1.bits[2] != value_2.bits[2] ||
             value_1.bits[3] != value_2.bits[3]) {
    is_equal = FALSE;
  }

  return is_equal;
}

int s21_is_not_equal(s21_decimal num1, s21_decimal num2) {
  return !(s21_is_equal(num1, num2));
}

int s21_is_greater_or_equal(s21_decimal num1, s21_decimal num2) {
  return (s21_is_greater(num1, num2) || s21_is_equal(num1, num2));
}

int s21_is_less(s21_decimal num1, s21_decimal num2) {
  return s21_is_greater(num2, num1);
}

int s21_is_less_or_equal(s21_decimal num1, s21_decimal num2) {
  return (s21_is_less(num1, num2) || s21_is_equal(num1, num2));
}

void minus_scale(s21_decimal *a) {
  s21_decimal ten = {{10u, 0, 0, 0}};
  if (last_bit(*a) < 32 && a->bits[0] < 10) a->bits[0] = 0;
  s21_decimal remainder;
  bit_division(*a, ten, &remainder, a);
}

void bit_division(s21_decimal number1, s21_decimal number2,
                  s21_decimal *remainder, s21_decimal *res) {
  clear_bits(remainder);
  clear_bits(res);
  for (int i = last_bit(number1); i >= 0; i--) {
    if (get_bit(number1, i)) set_bit(remainder, 0, 1);
    if (s21_is_greater_or_equal(*remainder, number2) == TRUE) {
      bit_subtraction(*remainder, number2, remainder);
      if (i != 0) shift_left(remainder, 1);
      if (get_bit(number1, i - 1)) set_bit(remainder, 0, 1);
      shift_left(res, 1);
      set_bit(res, 0, 1);
    } else {
      shift_left(res, 1);
      if (i != 0) shift_left(remainder, 1);
      if ((i - 1) >= 0 && get_bit(number1, i - 1)) set_bit(remainder, 0, 1);
    }
  }
}

void scale_decrease(s21_decimal *a) {
  s21_decimal ten = {{10u, 0, 0, 0}};
  if (last_bit(*a) < 32 && a->bits[0] < 10) a->bits[0] = 0;
  s21_decimal remainder;
  bit_division(*a, ten, &remainder, a);
}

void scale_equalize(s21_decimal *value_1, s21_decimal *value_2) {
  int sign1 = get_sign(value_1);
  int sign2 = get_sign(value_2);
  s21_decimal ten = {{10u, 0, 0, 0}};
  if (get_scale(value_1) < get_scale(value_2)) {
    int difference = get_scale(value_2) - get_scale(value_1);
    if (last_bit(*value_1) + difference <= 95 &&
        last_bit(*value_2) + difference <= 95) {
      for (int i = 0; i < difference; i++) {
        bit_multiplication(*value_1, ten, value_1);
      }
      set_scale(value_1, get_scale(value_2));
    } else {
      for (int i = 0; i < difference; i++) {
        scale_decrease(value_2);
      }
      set_scale(value_2, get_scale(value_1));
    }
  } else {
    int difference = get_scale(value_1) - get_scale(value_2);
    if (last_bit(*value_2) + difference <= 95 &&
        last_bit(*value_1) + difference <= 95) {
      for (int i = 0; i < difference; i++) {
        bit_multiplication(*value_2, ten, value_2);
      }
      set_scale(value_2, get_scale(value_1));
    } else {
      for (int i = 0; i < difference; i++) {
        scale_decrease(value_1);
      }
      set_scale(value_1, get_scale(value_2));
    }
  }
  set_sign(value_1, sign1);
  set_sign(value_2, sign2);
}

void bit_multiplication(s21_decimal val1, s21_decimal ten, s21_decimal *res) {
  clear_bits(res);
  s21_decimal tmp;
  int lastbit = last_bit(val1);
  for (int i = 0; i <= lastbit; i++) {
    clear_bits(&tmp);
    int val_bit = get_bit(val1, i);
    if (val_bit) {
      tmp = ten;
      shift_left(&tmp, i);
      bit_addition(*res, tmp, res);
    }
  }
}

int s21_truncate(s21_decimal value, s21_decimal *result) {
  clear_bits(result);
  s21_decimal ten = {{10, 0, 0, 0}};
  s21_decimal tmp = {{0, 0, 0, 0}};
  int sign = get_sign(&value);
  int scale = get_scale(&value);
  if (!scale)
    *result = value;
  else {
    for (int i = scale; i > 0; i--) {
      bit_division(value, ten, &tmp, result);
      value = *result;
    }
  }
  if (sign) set_sign(result, 1);
  return OK;
}

int s21_round(s21_decimal value, s21_decimal *result) {
  clear_bits(result);
  int return_value = OK;
  int sign = get_sign(&value);
  set_sign(&value, 0);

  s21_decimal tmp = {{0}};
  s21_truncate(value, &tmp);
  s21_decimal tmp_copy = tmp;
  s21_sub(value, tmp, &tmp);

  s21_decimal zero_five = {{5, 0, 0, 0}};
  s21_decimal one = {{1, 0, 0, 0}};
  set_scale(&zero_five, 1);
  if (s21_is_greater_or_equal(tmp, zero_five)) {
    return_value = s21_add(tmp_copy, one, result);
  } else {
    *result = tmp_copy;
  }
  if (sign) set_sign(result, 1);
  return return_value;
}

int s21_negate(s21_decimal value, s21_decimal *result) {
  *result = value;
  int sign = get_sign(result);
  if (sign) {
    set_sign(result, 0);
  } else {
    set_sign(result, 1);
  }
  return OK;
}

int s21_floor(s21_decimal value, s21_decimal *result) {
  clear_bits(result);
  int return_value = OK;
  s21_truncate(value, result);
  s21_decimal one = {{1, 0, 0, 0}};
  if (get_sign(&value)) {
    return_value = s21_sub(*result, one, result);
  }
  return return_value;
}

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  int error_code = OK;
  if (dst) {
    clear_bits(dst);
    if (src < 0) {
      set_sign(dst, 1);
      src *= -1;
    }
    dst->bits[0] = src;
  } else {
    error_code = CONVERTING_ERROR;
  }
  return error_code;
}

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  *dst = 0;
  int error_code = s21_truncate(src, &src);
  if (error_code != OK || src.bits[1] || src.bits[2] ||
      src.bits[0] > 2147483647u) {
    error_code = CONVERTING_ERROR;
  } else {
    *dst = src.bits[0];
    if (get_sign(&src)) *dst *= -1;
  }
  return error_code;
}

int s21_from_decimal_to_float(s21_decimal src, float *dst) {
  double tmp = 0;
  int exp = 0;
  for (int i = 0; i < 96; i++)
    if (((src.bits[i / 32]) & (1 << i % 32)) != 0) tmp += pow(2, i);
  if ((exp = (src.bits[3] & ~SIGN) >> 16) > 0) {
    for (int i = exp; i > 0; i--) tmp /= 10;
  }
  *dst = (float)tmp;
  *dst *= src.bits[3] >> 31 ? -1 : 1;
  return OK;
}

int get_float_sign(float *src) { return *(int *)src >> 31; }

int get_float_exp(float *src) { return ((*(int *)src & ~SIGN) >> 23) - 127; }

int s21_from_float_to_decimal(float src, s21_decimal *dst) {
  clear_bits(dst);
  int return_value = SUCCESS;

  if (isinf(src) || isnan(src)) {
    return_value = CONVERTING_ERROR;
  } else {
    if (src != 0) {
      int sign = get_float_sign(&src), exp = get_float_exp(&src);
      double temp = (double)fabs(src);
      int off = 0;
      for (; off < 28 && (int)temp / (int)pow(2, 21) == 0; temp *= 10, off++) {
      }
      temp = round(temp);
      if (off <= 28 && (exp > -94 && exp < 96)) {
        floatbits mant;
        temp = (float)temp;
        for (; fmod(temp, 10) == 0 && off > 0; off--, temp /= 10) {
        }
        mant.fl = temp;
        exp = get_float_exp(&mant.fl);
        dst->bits[exp / 32] |= 1 << exp % 32;
        for (int i = exp - 1, j = 22; j >= 0; i--, j--)
          if ((mant.ui & (1 << j)) != 0) dst->bits[i / 32] |= 1 << i % 32;
        dst->bits[3] = (sign << 31) | (off << 16);
      } else
        return_value = CONVERTING_ERROR;
    }
  }
  return return_value;
}