/* Returns whether given filename is invalid or not */

int invalid_filename(char *name) {
  int i = 0;
  while (((*name) >= 'a' && (*name) <= 'z') ||
         ((*name) >= '0' && (*name) <= '9') ||
         (*name == '.')) {
    i++;
    name++;
  }
  return (*name) || (i != 8);
}
