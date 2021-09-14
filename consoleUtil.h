/* Copyright (c) Microsoft Corporation. All rights reserved. */

#pragma once
#ifndef _CONSOLEUTIL_
#define _CONSOLEUTIL_

#include "common.h"
VOID DeleteCurrentLine();
VOID OverwriteCurrentLine();
VOID UpdatePercentageDisplay(ULONG Numerator, ULONG Denominator);

#endif // _CONSOLEUTIL_
