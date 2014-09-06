/* This program reads a Free42 skin layout, and grows all the keys'
 * sensitive rectangles by the maximum amount.
 * The idea is that having large sensitive rectangles around all keys
 * makes the keys easier to hit; less accurate pointing means less strain
 * and therefor, no more RSI. Or so we hope!
 * NOTE: if your input layout has bad sensitive rectangles (i.e. not
 * properly aligned with the actual keys), the result after converting
 * it using this program will also have poorly aligned sensitive
 * rectangles... The program is not smart enough to actually look at
 * your screen bitmap!
 */

#include <stdio.h>
#include <string.h>

char lines[1000][1000];
typedef struct {
    int left, right, top, bottom, line;
} key_spec;
key_spec keys[256];

int max(int x, int y) {
    return x > y ? x : y;
}

int min(int x, int y) {
    return x < y ? x : y;
}

void setkey(int key, int left, int right, int top, int bottom) {
    keys[key].left = left;
    keys[key].right = right;
    keys[key].top = top;
    keys[key].bottom = bottom;
}

int main() {
    char buf[1000];
    int line = 0;
    int key;
    int i, margin, edge;

    int vleft, vtop1, vtop2, vtop3, vtop4, vtop5, vbot1, vbot2, vbot3, vbot4, vright;
    int htop, h1, h2, h3, h4, h5, h6, hbot;

    for (i = 0; i < 256; i++)
        keys[i].line = 0;

    while (fgets(buf, 1000, stdin) != NULL) {
        int x, y, width, height, chars;

        line++;
        if (strncasecmp(buf, "key:", 4) != 0) {
            strcpy(lines[line], buf);
            continue;
        }
        if (sscanf(buf + 4, "%d %d , %d , %d , %d%n", &key, &x, &y, &width, &height, &chars) != 5) {
            fprintf(stderr, "Bad key spec in line %d; ignoring.\n", line);
            strcpy(lines[line], buf);
            continue;
        }
        keys[key].left = x;
        keys[key].right = x + width;
        keys[key].top = y;
        keys[key].bottom = y + height;
        keys[key].line = line;
        strcpy(lines[line], buf + chars + 4);
    }

    /* Find the positions of the vertical lines separating the keys
     * in the top 3 rows of the keyboard.
     */
    vtop1 = (keys[1].right + keys[2].left + keys[7].right + keys[8].left) / 4;
    vtop2 = (keys[2].right + keys[3].left + keys[8].right + keys[9].left + keys[13].right + keys[14].left) / 6;
    vtop3 = (keys[3].right + keys[4].left + keys[9].right + keys[10].left + keys[14].right + keys[15].left) / 6;
    vtop4 = (keys[4].right + keys[5].left + keys[10].right + keys[11].left + keys[15].right + keys[16].left) / 6;
    vtop5 = (keys[5].right + keys[6].left + keys[11].right + keys[12].left + keys[16].right + keys[17].left) / 6;

    /* Find the positions of the vertical lines separating the keys
     * in the bottom 4 rows of the keyboard.
     */
    vbot1 = (keys[18].right + keys[19].left + keys[23].right + keys[24].left + keys[28].right + keys[29].left + keys[33].right + keys[34].left) / 8;
    vbot2 = (keys[19].right + keys[20].left + keys[24].right + keys[25].left + keys[29].right + keys[30].left + keys[34].right + keys[35].left) / 8;
    vbot3 = (keys[20].right + keys[21].left + keys[25].right + keys[26].left + keys[30].right + keys[31].left + keys[35].right + keys[36].left) / 8;
    vbot4 = (keys[21].right + keys[22].left + keys[26].right + keys[27].left + keys[31].right + keys[32].left + keys[36].right + keys[37].left) / 8;

    /* Set the left margin to be as far as possible to the right, while still
     * extending all the leftmost keys' left edges at least as far as their
     * right edges.
     */
    margin = vtop1 - keys[1].right;
    margin = max(margin, vtop1 - keys[7].right);
    margin = max(margin, vtop2 - keys[13].right);
    margin = max(margin, vbot1 - keys[18].right);
    margin = max(margin, vbot1 - keys[23].right);
    margin = max(margin, vbot1 - keys[28].right);
    margin = max(margin, vbot1 - keys[33].right);
    edge = keys[1].left;
    edge = min(edge, keys[7].left);
    edge = min(edge, keys[13].left);
    edge = min(edge, keys[18].left);
    edge = min(edge, keys[23].left);
    edge = min(edge, keys[28].left);
    edge = min(edge, keys[33].left);
    vleft = edge - margin;

    /* Set the right margin to be as far as possible to the left, while still
     * extending all the rightmost keys' right edges at least as far as their
     * left edges.
     */
    margin = keys[6].left - vtop5;
    margin = min(margin, keys[12].left - vtop5);
    margin = min(margin, keys[17].left - vtop5);
    margin = min(margin, keys[22].left - vbot4);
    margin = min(margin, keys[27].left - vbot4);
    margin = min(margin, keys[32].left - vbot4);
    margin = min(margin, keys[37].left - vbot4);
    edge = keys[6].right;
    edge = max(edge, keys[12].right);
    edge = max(edge, keys[17].right);
    edge = max(edge, keys[22].right);
    edge = max(edge, keys[27].right);
    edge = max(edge, keys[32].right);
    edge = max(edge, keys[37].right);
    vright = edge + margin;

    /* Find the positions of the horizontal lines separating the keys
     */
    h1 = (keys[1].bottom + keys[2].bottom + keys[3].bottom + keys[4].bottom + keys[5].bottom + keys[6].bottom
            + keys[7].top + keys[8].top + keys[9].top + keys[10].top + keys[11].top + keys[12].top) / 12;
    h2 = ((keys[7].bottom + keys[8].bottom + keys[9].bottom + keys[10].bottom + keys[11].bottom + keys[12].bottom) * 5
            + (keys[13].top + keys[14].top + keys[15].top + keys[16].top + keys[17].top) * 6) / 60;
    h3 = (keys[13].bottom + keys[14].bottom + keys[15].bottom + keys[16].bottom + keys[17].bottom
            + keys[18].top + keys[19].top + keys[20].top + keys[21].top + keys[22].top) / 10;
    h4 = (keys[18].bottom + keys[19].bottom + keys[20].bottom + keys[21].bottom + keys[22].bottom
            + keys[23].top + keys[24].top + keys[25].top + keys[26].top + keys[27].top) / 10;
    h5 = (keys[23].bottom + keys[24].bottom + keys[25].bottom + keys[26].bottom + keys[27].bottom
            + keys[28].top + keys[29].top + keys[30].top + keys[31].top + keys[32].top) / 10;
    h6 = (keys[28].bottom + keys[29].bottom + keys[30].bottom + keys[31].bottom + keys[32].bottom
            + keys[33].top + keys[34].top + keys[35].top + keys[36].top + keys[37].top) / 10;

    /* Set the top margin to be as far down as possible, while still
     * extending all the topmost keys' top edges at least as far as their
     * bottom edges.
     */
    margin = h1 - keys[1].bottom;
    margin = max(margin, h1 - keys[2].bottom);
    margin = max(margin, h1 - keys[3].bottom);
    margin = max(margin, h1 - keys[4].bottom);
    margin = max(margin, h1 - keys[5].bottom);
    margin = max(margin, h1 - keys[6].bottom);
    edge = keys[1].top;
    edge = min(edge, keys[2].top);
    edge = min(edge, keys[3].top);
    edge = min(edge, keys[4].top);
    edge = min(edge, keys[5].top);
    edge = min(edge, keys[6].top);
    htop = edge - margin;

    /* Set the bottom margin to be as far up as possible, while still
     * extending all the bottommost keys' bottom edges at least as far as their
     * top edges.
     */
    margin = keys[33].top - h6;;
    margin = min(margin, keys[34].top - h6);
    margin = min(margin, keys[35].top - h6);
    margin = min(margin, keys[36].top - h6);
    margin = min(margin, keys[37].top - h6);
    edge = keys[33].bottom;
    edge = max(edge, keys[34].bottom);
    edge = max(edge, keys[35].bottom);
    edge = max(edge, keys[36].bottom);
    edge = max(edge, keys[37].bottom);
    hbot = edge + margin;

    /* Set sensitive rectangles to their new dimensions */
    setkey(1, vleft, vtop1, htop, h1);
    setkey(2, vtop1, vtop2, htop, h1);
    setkey(3, vtop2, vtop3, htop, h1);
    setkey(4, vtop3, vtop4, htop, h1);
    setkey(5, vtop4, vtop5, htop, h1);
    setkey(6, vtop5, vright, htop, h1);
    setkey(7, vleft, vtop1, h1, h2);
    setkey(8, vtop1, vtop2, h1, h2);
    setkey(9, vtop2, vtop3, h1, h2);
    setkey(10, vtop3, vtop4, h1, h2);
    setkey(11, vtop4, vtop5, h1, h2);
    setkey(12, vtop5, vright, h1, h2);
    setkey(13, vleft, vtop2, h2, h3);
    setkey(14, vtop2, vtop3, h2, h3);
    setkey(15, vtop3, vtop4, h2, h3);
    setkey(16, vtop4, vtop5, h2, h3);
    setkey(17, vtop5, vright, h2, h3);
    setkey(18, vleft, vbot1, h3, h4);
    setkey(19, vbot1, vbot2, h3, h4);
    setkey(20, vbot2, vbot3, h3, h4);
    setkey(21, vbot3, vbot4, h3, h4);
    setkey(22, vbot4, vright, h3, h4);
    setkey(23, vleft, vbot1, h4, h5);
    setkey(24, vbot1, vbot2, h4, h5);
    setkey(25, vbot2, vbot3, h4, h5);
    setkey(26, vbot3, vbot4, h4, h5);
    setkey(27, vbot4, vright, h4, h5);
    setkey(28, vleft, vbot1, h5, h6);
    setkey(29, vbot1, vbot2, h5, h6);
    setkey(30, vbot2, vbot3, h5, h6);
    setkey(31, vbot3, vbot4, h5, h6);
    setkey(32, vbot4, vright, h5, h6);
    setkey(33, vleft, vbot1, h6, hbot);
    setkey(34, vbot1, vbot2, h6, hbot);
    setkey(35, vbot2, vbot3, h6, hbot);
    setkey(36, vbot3, vbot4, h6, hbot);
    setkey(37, vbot4, vright, h6, hbot);

    /* Print the modified skin file */
    key = 1;
    for (i = 1; i <= line; i++) {
        if (keys[key].line == i) {
            printf("Key: %d %d,%d,%d,%d", key, keys[key].left, keys[key].top, keys[key].right - keys[key].left, keys[key].bottom - keys[key].top);
            key++;
        }
        printf(lines[i]);
    }
    fflush(stdout);
}
