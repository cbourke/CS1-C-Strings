/*
 * cmocka test suite for game_functions.c
 * 
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <cmocka.h>

/* Functions under test */
void initializeBlankString(int length, char *s);
void printWithSpaces(const char *s);
int  revealGuessedLetter(const char *s, char *t, char c);
int  checkGuess(const char *s, const char *t);

#define BUF_SIZE 128
#define GUARD    'X'

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* Render n bytes of buf printably: NUL shown as \0. */
static void render(const char *buf, size_t n, char *out, size_t outsz)
{
    size_t o = 0;
    for (size_t i = 0; i < n && o + 3 < outsz; i++) {
        if (buf[i] == '\0') {
            out[o++] = '\\';
            out[o++] = '0';
        } else if (isprint((unsigned char)buf[i])) {
            out[o++] = buf[i];
        } else {
            out[o++] = '?';
        }
    }
    out[o] = '\0';
}

/* ---- initializeBlankString ---------------------------------------- */

/*
 * Fill a guarded buffer (optionally with prefill text), call the
 * function, and verify: first `length` bytes are '_', byte `length`
 * is NUL, and every other byte is exactly as it was before the call.
 */
static void check_init(int length, const char *prefill)
{
    char buf[BUF_SIZE], before[BUF_SIZE], expected[BUF_SIZE];
    char r_before[2 * BUF_SIZE + 1];
    char r_exp[2 * BUF_SIZE + 1];
    char r_act[2 * BUF_SIZE + 1];
    size_t show = (size_t)length + 3;

    if (show > BUF_SIZE) {
        show = BUF_SIZE;
    }

    memset(buf, GUARD, sizeof buf);
    if (prefill != NULL) {
        memcpy(buf, prefill, strlen(prefill) + 1);
    }
    memcpy(before, buf, sizeof buf);

    memcpy(expected, buf, sizeof buf);
    memset(expected, '_', (size_t)length);
    expected[length] = '\0';

    initializeBlankString(length, buf);

    if (memcmp(buf, expected, sizeof buf) != 0) {
        render(before,   show, r_before, sizeof r_before);
        render(expected, show, r_exp,    sizeof r_exp);
        render(buf,      show, r_act,    sizeof r_act);
        fail_msg("initializeBlankString(%d, s)\n"
                 "  buffer before: [%s]\n"
                 "  expected:      [%s]\n"
                 "  actual:        [%s]\n"
                 "  (\\0 = NUL byte, X = guard byte that must stay untouched)",
                 length, r_before, r_exp, r_act);
    }
}

/* ---- printWithSpaces ---------------------------------------------- */

/* Run printWithSpaces(s) with stdout redirected, return what it printed. */
static void capture_print(const char *s, char *out, size_t outsz)
{
    FILE *tmp;
    int saved;
    size_t got;

    fflush(stdout);
    saved = dup(STDOUT_FILENO);
    tmp = tmpfile();
    if (saved < 0 || tmp == NULL) {
        fail_msg("test harness error: could not redirect stdout");
    }

    dup2(fileno(tmp), STDOUT_FILENO);
    printWithSpaces(s);
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    rewind(tmp);
    got = fread(out, 1, outsz - 1, tmp);
    out[got] = '\0';
    fclose(tmp);
}

static void check_print(const char *input, const char *expected)
{
    char actual[1024];

    capture_print(input, actual, sizeof actual);

    if (strcmp(actual, expected) != 0) {
        fail_msg("printWithSpaces(\"%s\")\n"
                 "  expected output: \"%s\" (%zu chars)\n"
                 "  actual output:   \"%s\" (%zu chars)",
                 input, expected, strlen(expected),
                 actual, strlen(actual));
    }
}

/* ---- revealGuessedLetter ------------------------------------------ */

/*
 * Calls revealGuessedLetter(solution, t, c) where t starts as `start`.
 * Verifies the resulting t, the return value (skipped if exp_ret < 0),
 * that the solution string was not modified, and that no byte of t
 * beyond its terminator was touched.
 */
static void check_reveal(const char *solution, const char *start, char c,
                         const char *exp_t, int exp_ret)
{
    char t[BUF_SIZE];
    char sol_copy[BUF_SIZE];
    char cdisp[3];
    size_t len = strlen(start);
    int ret;

    if (c == '\0') {
        strcpy(cdisp, "\\0");
    } else {
        cdisp[0] = c;
        cdisp[1] = '\0';
    }

    memset(t, GUARD, sizeof t);
    memcpy(t, start, len + 1);
    strcpy(sol_copy, solution);

    ret = revealGuessedLetter(sol_copy, t, c);

    if (strcmp(t, exp_t) != 0 || (exp_ret >= 0 && ret != exp_ret)) {
        if (exp_ret >= 0) {
            fail_msg("revealGuessedLetter(\"%s\", \"%s\", '%s')\n"
                     "  expected: return %d, t = \"%s\"\n"
                     "  actual:   return %d, t = \"%s\"",
                     solution, start, cdisp, exp_ret, exp_t, ret, t);
        } else {
            fail_msg("revealGuessedLetter(\"%s\", \"%s\", '%s')\n"
                     "  expected: t = \"%s\" (return value not checked)\n"
                     "  actual:   return %d, t = \"%s\"",
                     solution, start, cdisp, exp_t, ret, t);
        }
    }

    if (strcmp(sol_copy, solution) != 0) {
        fail_msg("revealGuessedLetter(\"%s\", \"%s\", '%s')\n"
                 "  the solution string must not be altered\n"
                 "  expected solution: \"%s\"\n"
                 "  actual solution:   \"%s\"",
                 solution, start, cdisp, solution, sol_copy);
    }

    for (size_t i = len + 1; i < BUF_SIZE; i++) {
        if (t[i] != GUARD) {
            fail_msg("revealGuessedLetter(\"%s\", \"%s\", '%s')\n"
                     "  wrote past the end of t: byte %zu should be the "
                     "untouched guard 'X' but is 0x%02x",
                     solution, start, cdisp, i, (unsigned char)t[i]);
        }
    }
}

/* ---- checkGuess --------------------------------------------------- */

static void check_guess(const char *a, const char *b, int expected)
{
    int actual = checkGuess(a, b);

    if (actual != expected) {
        fail_msg("checkGuess(\"%s\", \"%s\")\n"
                 "  expected return: %d\n"
                 "  actual return:   %d",
                 a, b, expected, actual);
    }
}

/* ================================================================== */
/* initializeBlankString tests                                        */
/* ================================================================== */

static void test_init_typical_length_5(void **state)
{
    (void)state;
    check_init(5, NULL);
}

static void test_init_length_8_dinosaur(void **state)
{
    (void)state;
    check_init(8, NULL);
}

static void test_init_length_1(void **state)
{
    (void)state;
    check_init(1, NULL);
}

static void test_init_length_0_only_terminates(void **state)
{
    (void)state;
    /* Corner case: nothing to fill, but s[0] must become '\0'. */
    check_init(0, "junk");
}

static void test_init_overwrites_existing_letters(void **state)
{
    (void)state;
    check_init(8, "dinosaur");
}

static void test_init_prefill_longer_than_length(void **state)
{
    (void)state;
    /* Only the first 3 chars and the NUL change; "o world" tail stays. */
    check_init(3, "hello world");
}

static void test_init_prefill_already_underscores(void **state)
{
    (void)state;
    check_init(6, "______");
}

static void test_init_max_length_fits_buffer(void **state)
{
    (void)state;
    /* Corner case: length 127 uses every byte of the 128-byte buffer. */
    check_init(BUF_SIZE - 1, NULL);
}

static void test_init_strlen_equals_length(void **state)
{
    char buf[BUF_SIZE];
    size_t actual_len;
    size_t underscores;

    (void)state;
    memset(buf, GUARD, sizeof buf);
    initializeBlankString(26, buf);
    actual_len = strlen(buf);
    underscores = strspn(buf, "_");

    if (actual_len != 26 || underscores != 26) {
        fail_msg("initializeBlankString(26, s)\n"
                 "  expected: strlen(s) == 26 and 26 leading underscores\n"
                 "  actual:   strlen(s) == %zu and %zu leading underscores",
                 actual_len, underscores);
    }
}

static void test_init_long_prefill_guard_after_terminator(void **state)
{
    (void)state;
    check_init(10, "abcdefghijklmnopqrstuvwxyz");
}

static void test_init_called_twice_is_idempotent(void **state)
{
    char buf[BUF_SIZE];

    (void)state;
    memset(buf, GUARD, sizeof buf);
    initializeBlankString(7, buf);
    initializeBlankString(7, buf);

    if (strcmp(buf, "_______") != 0) {
        fail_msg("initializeBlankString(7, s) called twice\n"
                 "  expected: \"_______\"\n"
                 "  actual:   \"%s\"", buf);
    }
}

static void test_init_reinitialize_with_shorter_length(void **state)
{
    char buf[BUF_SIZE];

    (void)state;
    memset(buf, GUARD, sizeof buf);
    initializeBlankString(8, buf);
    initializeBlankString(3, buf);

    if (strcmp(buf, "___") != 0) {
        fail_msg("initializeBlankString(8, s) then initializeBlankString(3, s)\n"
                 "  expected: \"___\"\n"
                 "  actual:   \"%s\"", buf);
    }
}

/*
 * Error handling: a negative length is invalid.  The provided
 * implementation executes s[length] = '\0' with length < 0, writing
 * BEFORE the buffer (out of bounds).  This test places s in the middle
 * of an arena and expects the byte(s) before it to remain untouched.
 * It is EXPECTED TO FAIL against the code as given; a guard such as
 * "if (length < 0) return;" fixes it.
 */
static void test_init_negative_length_no_out_of_bounds_write(void **state)
{
    char arena[16];
    char expected[16];
    char r_exp[64];
    char r_act[64];
    char *s = arena + 4;

    (void)state;
    memset(arena, GUARD, sizeof arena);
    memset(expected, GUARD, sizeof expected);

    initializeBlankString(-1, s);

    if (memcmp(arena, expected, sizeof arena) != 0) {
        render(expected, sizeof expected, r_exp, sizeof r_exp);
        render(arena,    sizeof arena,    r_act, sizeof r_act);
        fail_msg("initializeBlankString(-1, s)   [s = arena + 4]\n"
                 "  a negative length must not modify memory\n"
                 "  expected arena: [%s]\n"
                 "  actual arena:   [%s]\n"
                 "  (a \\0 at index 3 means s[-1] was written)",
                 r_exp, r_act);
    }
}

/* ================================================================== */
/* printWithSpaces tests                                              */
/* ================================================================== */

static void test_print_basic_word(void **state)
{
    (void)state;
    check_print("abc", "a b c ");
}

static void test_print_single_char(void **state)
{
    (void)state;
    check_print("a", "a ");
}

static void test_print_empty_string(void **state)
{
    (void)state;
    check_print("", "");
}

static void test_print_all_blanks(void **state)
{
    (void)state;
    check_print("_____", "_ _ _ _ _ ");
}

static void test_print_partially_revealed(void **state)
{
    (void)state;
    check_print("_a_a_", "_ a _ a _ ");
}

static void test_print_mixed_case(void **state)
{
    (void)state;
    check_print("Hello", "H e l l o ");
}

static void test_print_digits(void **state)
{
    (void)state;
    check_print("2024", "2 0 2 4 ");
}

static void test_print_punctuation(void **state)
{
    (void)state;
    check_print("!?-.", "! ? - . ");
}

static void test_print_embedded_space(void **state)
{
    (void)state;
    /* 'a', then the space char plus its separator, then 'b' */
    check_print("a b", "a   b ");
}

static void test_print_percent_not_interpreted(void **state)
{
    (void)state;
    /* Corner case: '%' and 'd' must be printed literally. */
    check_print("%d", "% d ");
}

static void test_print_no_trailing_newline(void **state)
{
    char out[64];

    (void)state;
    capture_print("hi", out, sizeof out);

    if (strlen(out) > 0 && out[strlen(out) - 1] == '\n') {
        fail_msg("printWithSpaces(\"hi\")\n"
                 "  expected: no newline printed (output \"h i \")\n"
                 "  actual:   output ends with a newline");
    }
}

static void test_print_long_string(void **state)
{
    char input[51];
    char expected[101];

    (void)state;
    for (int i = 0; i < 50; i++) {
        input[i] = (char)('a' + (i % 26));
        expected[2 * i] = input[i];
        expected[2 * i + 1] = ' ';
    }
    input[50] = '\0';
    expected[100] = '\0';

    check_print(input, expected);
}

static void test_print_does_not_modify_input(void **state)
{
    char input[] = "dino";
    char out[64];

    (void)state;
    capture_print(input, out, sizeof out);

    if (strcmp(input, "dino") != 0) {
        fail_msg("printWithSpaces(\"dino\")\n"
                 "  expected: input string left unchanged (\"dino\")\n"
                 "  actual:   input string is now \"%s\"", input);
    }
}

/* ================================================================== */
/* revealGuessedLetter tests                                          */
/* ================================================================== */

static void test_reveal_spec_example(void **state)
{
    (void)state;
    check_reveal("dinosaur", "________", 'a', "_____a__", 1);
}

static void test_reveal_letter_not_in_solution(void **state)
{
    (void)state;
    check_reveal("dinosaur", "________", 'z', "________", 0);
}

static void test_reveal_multiple_occurrences(void **state)
{
    (void)state;
    check_reveal("banana", "______", 'a', "_a_a_a", 1);
}

static void test_reveal_first_letter(void **state)
{
    (void)state;
    check_reveal("dinosaur", "________", 'd', "d_______", 1);
}

static void test_reveal_last_letter(void **state)
{
    (void)state;
    check_reveal("dinosaur", "________", 'r', "_______r", 1);
}

static void test_reveal_all_same_letter(void **state)
{
    (void)state;
    check_reveal("aaaa", "____", 'a', "aaaa", 1);
}

static void test_reveal_builds_on_previous_guesses(void **state)
{
    (void)state;
    check_reveal("dinosaur", "_____a__", 'o', "___o_a__", 1);
}

static void test_reveal_already_revealed_letter_keeps_string(void **state)
{
    (void)state;
    /*
     * Ambiguous in the spec: "return 1 if any letters were changed".
     * Re-guessing an already-revealed letter changes nothing, but the
     * provided code returns 1.  So only the string is checked here
     * (exp_ret = -1 means "don't check the return value").
     */
    check_reveal("dinosaur", "_____a__", 'a', "_____a__", -1);
}

static void test_reveal_is_case_sensitive(void **state)
{
    (void)state;
    check_reveal("Dinosaur", "________", 'd', "________", 0);
    check_reveal("Dinosaur", "________", 'D', "D_______", 1);
}

static void test_reveal_empty_strings(void **state)
{
    (void)state;
    check_reveal("", "", 'a', "", 0);
}

static void test_reveal_single_char_match(void **state)
{
    (void)state;
    check_reveal("a", "_", 'a', "a", 1);
}

static void test_reveal_single_char_no_match(void **state)
{
    (void)state;
    check_reveal("a", "_", 'b', "_", 0);
}

static void test_reveal_nul_guess_changes_nothing(void **state)
{
    (void)state;
    /* Corner case: the terminator must never count as a match. */
    check_reveal("dinosaur", "________", '\0', "________", 0);
}

static void test_reveal_space_character(void **state)
{
    (void)state;
    check_reveal("a b", "___", ' ', "_ _", 1);
}

static void test_reveal_digits_and_punctuation(void **state)
{
    (void)state;
    check_reveal("r2-d2", "_____", '2', "_2__2", 1);
    check_reveal("r2-d2", "_____", '-', "__-__", 1);
}

static void test_reveal_full_game_sequence(void **state)
{
    char t[BUF_SIZE];
    int r;

    (void)state;
    strcpy(t, "________");

    r = revealGuessedLetter("dinosaur", t, 'z');
    if (r != 0 || strcmp(t, "________") != 0) {
        fail_msg("game step 1: guess 'z' on \"________\" (solution \"dinosaur\")\n"
                 "  expected: return 0, t = \"________\"\n"
                 "  actual:   return %d, t = \"%s\"", r, t);
    }
    r = revealGuessedLetter("dinosaur", t, 'o');
    if (r != 1 || strcmp(t, "___o____") != 0) {
        fail_msg("game step 2: guess 'o' on \"________\" (solution \"dinosaur\")\n"
                 "  expected: return 1, t = \"___o____\"\n"
                 "  actual:   return %d, t = \"%s\"", r, t);
    }
    r = revealGuessedLetter("dinosaur", t, 'r');
    if (r != 1 || strcmp(t, "___o___r") != 0) {
        fail_msg("game step 3: guess 'r' on \"___o____\" (solution \"dinosaur\")\n"
                 "  expected: return 1, t = \"___o___r\"\n"
                 "  actual:   return %d, t = \"%s\"", r, t);
    }
}

/* ================================================================== */
/* checkGuess tests                                                   */
/* ================================================================== */

static void test_check_equal_strings(void **state)
{
    (void)state;
    check_guess("dinosaur", "dinosaur", 1);
}

static void test_check_different_strings(void **state)
{
    (void)state;
    check_guess("dinosaur", "elephant", 0);
}

static void test_check_both_empty(void **state)
{
    (void)state;
    check_guess("", "", 1);
}

static void test_check_empty_vs_nonempty(void **state)
{
    (void)state;
    check_guess("", "dino", 0);
    check_guess("dino", "", 0);
}

static void test_check_prefix_is_not_equal(void **state)
{
    (void)state;
    check_guess("dino", "dinosaur", 0);
    check_guess("dinosaur", "dino", 0);
}

static void test_check_case_sensitive(void **state)
{
    (void)state;
    check_guess("Dinosaur", "dinosaur", 0);
}

static void test_check_differs_in_first_char(void **state)
{
    (void)state;
    check_guess("dinosaur", "binosaur", 0);
}

static void test_check_differs_in_last_char(void **state)
{
    (void)state;
    check_guess("dinosaur", "dinosaus", 0);
}

static void test_check_differs_in_middle_char(void **state)
{
    (void)state;
    check_guess("dinosaur", "dinesaur", 0);
}

static void test_check_trailing_space_matters(void **state)
{
    (void)state;
    check_guess("dino ", "dino", 0);
}

static void test_check_same_pointer(void **state)
{
    const char *word = "dinosaur";

    (void)state;
    check_guess(word, word, 1);
}

static void test_check_blank_vs_revealed(void **state)
{
    (void)state;
    check_guess("_a_a_", "_a_a_", 1);
    check_guess("_a_a_", "_____", 0);
}

static void test_check_single_chars(void **state)
{
    (void)state;
    check_guess("a", "a", 1);
    check_guess("a", "b", 0);
}

static void test_check_special_characters(void **state)
{
    (void)state;
    check_guess("r2-d2!", "r2-d2!", 1);
    check_guess("r2-d2!", "r2-d2?", 0);
}

static void test_check_long_strings(void **state)
{
    char a[101];
    char b[101];

    (void)state;
    memset(a, 'x', 100);
    memset(b, 'x', 100);
    a[100] = '\0';
    b[100] = '\0';

    if (checkGuess(a, b) != 1) {
        fail_msg("checkGuess(<100 x's>, <100 x's>)\n"
                 "  expected return: 1\n"
                 "  actual return:   %d", checkGuess(a, b));
    }

    b[99] = 'y';
    if (checkGuess(a, b) != 0) {
        fail_msg("checkGuess(<100 x's>, <99 x's then 'y'>)\n"
                 "  expected return: 0\n"
                 "  actual return:   %d", checkGuess(a, b));
    }
}

/* ================================================================== */

int main(void)
{
    const struct CMUnitTest tests[] = {
        /* initializeBlankString */
        cmocka_unit_test(test_init_typical_length_5),
        cmocka_unit_test(test_init_length_8_dinosaur),
        cmocka_unit_test(test_init_length_1),
        cmocka_unit_test(test_init_length_0_only_terminates),
        cmocka_unit_test(test_init_overwrites_existing_letters),
        cmocka_unit_test(test_init_prefill_longer_than_length),
        cmocka_unit_test(test_init_prefill_already_underscores),
        cmocka_unit_test(test_init_max_length_fits_buffer),
        cmocka_unit_test(test_init_strlen_equals_length),
        cmocka_unit_test(test_init_long_prefill_guard_after_terminator),
        cmocka_unit_test(test_init_called_twice_is_idempotent),
        cmocka_unit_test(test_init_reinitialize_with_shorter_length),
        cmocka_unit_test(test_init_negative_length_no_out_of_bounds_write),

        /* printWithSpaces */
        cmocka_unit_test(test_print_basic_word),
        cmocka_unit_test(test_print_single_char),
        cmocka_unit_test(test_print_empty_string),
        cmocka_unit_test(test_print_all_blanks),
        cmocka_unit_test(test_print_partially_revealed),
        cmocka_unit_test(test_print_mixed_case),
        cmocka_unit_test(test_print_digits),
        cmocka_unit_test(test_print_punctuation),
        cmocka_unit_test(test_print_embedded_space),
        cmocka_unit_test(test_print_percent_not_interpreted),
        cmocka_unit_test(test_print_no_trailing_newline),
        cmocka_unit_test(test_print_long_string),
        cmocka_unit_test(test_print_does_not_modify_input),

        /* revealGuessedLetter */
        cmocka_unit_test(test_reveal_spec_example),
        cmocka_unit_test(test_reveal_letter_not_in_solution),
        cmocka_unit_test(test_reveal_multiple_occurrences),
        cmocka_unit_test(test_reveal_first_letter),
        cmocka_unit_test(test_reveal_last_letter),
        cmocka_unit_test(test_reveal_all_same_letter),
        cmocka_unit_test(test_reveal_builds_on_previous_guesses),
        cmocka_unit_test(test_reveal_already_revealed_letter_keeps_string),
        cmocka_unit_test(test_reveal_is_case_sensitive),
        cmocka_unit_test(test_reveal_empty_strings),
        cmocka_unit_test(test_reveal_single_char_match),
        cmocka_unit_test(test_reveal_single_char_no_match),
        cmocka_unit_test(test_reveal_nul_guess_changes_nothing),
        cmocka_unit_test(test_reveal_space_character),
        cmocka_unit_test(test_reveal_digits_and_punctuation),
        cmocka_unit_test(test_reveal_full_game_sequence),

        /* checkGuess */
        cmocka_unit_test(test_check_equal_strings),
        cmocka_unit_test(test_check_different_strings),
        cmocka_unit_test(test_check_both_empty),
        cmocka_unit_test(test_check_empty_vs_nonempty),
        cmocka_unit_test(test_check_prefix_is_not_equal),
        cmocka_unit_test(test_check_case_sensitive),
        cmocka_unit_test(test_check_differs_in_first_char),
        cmocka_unit_test(test_check_differs_in_last_char),
        cmocka_unit_test(test_check_differs_in_middle_char),
        cmocka_unit_test(test_check_trailing_space_matters),
        cmocka_unit_test(test_check_same_pointer),
        cmocka_unit_test(test_check_blank_vs_revealed),
        cmocka_unit_test(test_check_single_chars),
        cmocka_unit_test(test_check_special_characters),
        cmocka_unit_test(test_check_long_strings),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
