
#ifndef _STAGE00_H_
#define _STAGE00_H_

#include "graphic.h"

void stage00(int);
void initStage00();

void drawSquare();
void drawN64Logo();

void breakInThisFunction();
void causeTLBFault();
void causeDivideByZeroException();
void causeOSError();
void hangThread();

#endif /* _STAGE00_H_ */
