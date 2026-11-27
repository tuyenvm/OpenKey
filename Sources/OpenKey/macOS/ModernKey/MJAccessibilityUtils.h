//
//  MJAccessibilityUtils.h
//  OpenKey
//
//  Created by Nguyen Tan Thong on 18/9/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
//  Source: https://github.com/Hammerspoon/hammerspoon/blob/master/Hammerspoon/MJAccessibilityUtils.h
//  License: MIT

#ifndef MJAccessibilityUtils_h

#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL MJAccessibilityIsEnabled(void);
void MJAccessibilityOpenPanel(void);

#ifdef __cplusplus
}
#endif

#define MJAccessibilityUtils_h


#endif /* MJAccessibilityUtils_h */
