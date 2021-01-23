#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
//    FILE *fp1 = fopen("dsds.xtxt", "r");
//    if (fp1 == NULL) {
//        printf("NULL\n");
//    }
    FILE *fp = fopen("test.txt", "rb+");
//    char buf[1024];
//    memset(buf, 0, sizeof(buf));
//    fseek(fp, 8, SEEK_SET);
//    int ret = fread(buf, 1, 10, fp);
//    printf("ret = %d\t%s\n", ret, buf);
//    char *p = "bbbb";
//    fseek(fp, 2, SEEK_SET);
//    fwrite(p, 1, strlen(p), fp);
//    fclose(fp);
//    fclose(fp1);
    char buf[10];
    buf[9] = '\0';
    int size = ftell(fp);
    printf("size=%d\n", size);
    fread(buf, 9, 1, fp);
    fclose(fp);
    printf("%s\n", buf);
}