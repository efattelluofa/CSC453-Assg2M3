int testFunc() {
    int localVar;
}

int localVar;
int globalVar;
int testFunc2(int hello) {
    int localVar;
    int globalVar;
    testFunc();
    testFunc();
    testFunc2();
    testFunc2();
}
/*
 *
 *
 *
 */
int testFunc3(/*   */int localVar, int globalVar) {
    int localVar;
    testFunc2();
}
