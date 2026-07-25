/* Returns whether given filename is invalid or not */

int invalid_filename(char *name) {
  int i = 0;
  while (((*name) >= 'a' && (*name) <= 'z') ||
         ((*name) >= '0' && (*name) <= '9')) {
    i++;
    name++;
  }
  return (!(i) || (i > 12) || ((*name) != '.') || ((*(name + 1)) != 'a')
          || ((*(name + 2)) != 'u') || ((*(name + 3)) != '\0'));
}
