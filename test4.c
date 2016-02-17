#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
 
// This test first forks 48 process plus the two forked in phase1 makes 50 
// Attempts to fork the 51st process which fails since there is not enough space
// Then marks process 48 for death. After P2_Startup finishes two more processes can be forked
// in slots 1 and 48 (since 48 was marked for death and later quit)
/* Correct Output:
 
P2_Startup
Pass - > Returned: -1, Not enough process space
PASS: process 48 marked for death
PASS
PASS
P5_Startup
1
P5_Finish
P5_Startup
48
P5_Finish
Goodbye.
*/
 
int P3_Startup(void *notused);
int P4_Startup(void *notused);
int P5_Startup(void *notused);
 
int P2_Startup(void *notused) 
{
    USLOSS_Console("P2_Startup\n");
    int i, result;
    
    for (i = 0; i < (P1_MAXPROC - 4); i++) {
        result = P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 5);
        if (result == -1) {
            USLOSS_Console("Fail returned: %d\n", result);
        }
    }
  
    result = P1_Fork("P4_Startup", P4_Startup, NULL, 4 *  USLOSS_MIN_STACK, 3);
    if (result == -1) {
         USLOSS_Console("Fail returned: %d\n", result);
    }
 
    result = P1_Fork("P4_Startup", P4_Startup, NULL, 4 *  USLOSS_MIN_STACK, 4);
    if (result == -1) {
         USLOSS_Console("Fail returned: %d\n", result);
    }
 
    result = P1_Fork("P3_Startup", P3_Startup, NULL, 4 *  USLOSS_MIN_STACK, 2);
    if (result == -1) {
            USLOSS_Console("Pass - > Returned: %d, Not enough process space\n", result);
    }
    
    P1_Kill(48);

    printf("%d\n",P1_GetState(48)); 
    if (P1_GetState(48) == 2) {
       USLOSS_Console("PASS: process 48 marked for death\n");
    }
 
    return 0;
}
 
int P3_Startup(void *notused) 
{
    return 0; 
}
 
int P4_Startup(void *notused) {
   int result = P1_Fork("P5_Startup", P5_Startup, NULL, 4 *  USLOSS_MIN_STACK, 5);
   if  (result == 1)
       USLOSS_Console("PASS\n");
    
   result = P1_Fork("P5_Startup", P5_Startup, NULL, 4 *  USLOSS_MIN_STACK, 5);
   if (result == 48) 
       USLOSS_Console("PASS\n");
  
   return 0;
}
 
int P5_Startup(void *notused) {
   USLOSS_Console("P5_Startup\n");
   USLOSS_Console("%d\n", P1_GetPID());
   USLOSS_Console("P5_Finish\n");
   return 0;
}
 
void setup(void) {
   // do nothing
}
 
void cleanup(void) {
    // Do nothing.
}
