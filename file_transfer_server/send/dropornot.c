/* Returns whether current packet should be transmitted */

extern int packselect[5][2];

int dropornot(int seq) {
  for (int i = 0; i < 5; i++) {
    if (seq == packselect[i][0] && packselect[i][1]) {
      packselect[i][1]--;
      return 1;
    }
  }
  return 0;
}
