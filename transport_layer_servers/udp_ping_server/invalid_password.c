/* Returns whether given password is invalid or not */

int invalid_password(char *pass) {
  int i = 0;
  while (((*pass) >= 'a' && (*pass) <= 'z') ||
         ((*pass) >= '0' && (*pass) <= '9')) {
    i++;
    pass++;
  }
  return (*pass) || (i != 4);
}
