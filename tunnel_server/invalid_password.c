/* Aakash Patel - pate2011 */
/* Returns whether given password is invalid or not */

int invalid_password(char *pass) {
  int i = 0;
  while (((*pass) >= 'a' && (*pass) <= 'z') ||
         ((*pass) >= 'A' && (*pass) <= 'Z')) {
    i++;
    pass++;
  }
  return (*pass) || (i != 5);
}
