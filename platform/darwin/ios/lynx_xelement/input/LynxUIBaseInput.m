// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIBaseInput.h>
#import <XElement/LynxUIBaseInputShadowNode.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxConverter+UI.h>
#import <Lynx/LynxColorUtils.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <XElement/LynxTextKeyListener.h>
#import <XElement/LynxDigitKeyListener.h>
#import <XElement/LynxDialerKeyListener.h>
#import <Lynx/LynxUnitUtils.h>

#define UNDEFINED_INT NSUIntegerMax
#define UNDEFINED_FLOAT CGFLOAT_MIN

@interface LynxUIBaseInput () <LynxFontFaceObserver>

@property (nonatomic, strong) NSString *preValue;
@property (nonatomic, strong) NSDictionary *preInputData;
@property (nonatomic, strong) NSString *inputFilterRegex;
@property (nonatomic, strong) id<LynxKeyListener> keyListener;

@end

@implementation LynxUIBaseInput

- (instancetype)init {
  if (self = [super init]) {
    _sendComposingInputEvent = YES;
    _maxLength = NSIntegerMax;
    [LynxPropsProcessor updateProp:@(14) withKey:@"font-size" forUI:self];
    [LynxPropsProcessor updateProp:@(14) withKey:@"placeholder-font-size" forUI:self];
    _fontWeight = UIFontWeightRegular;
    _placeholderFontWeight = UNDEFINED_FLOAT;
    _fontStyle = LynxFontStyleNormal;
    _placeholderFontStyle = UNDEFINED_INT;
    _fontFamily = nil;
    _placeholderFontFamily = nil;
    _placeholderFontSize = UNDEFINED_FLOAT;
    _inputAttrs = [NSMutableDictionary dictionary];
    _placeholderAttrs = [NSMutableDictionary dictionary];

    _inputParagraphStyle = [[NSMutableParagraphStyle alloc] init];
    _inputParagraphStyle.lineBreakMode = NSLineBreakByCharWrapping;
    
    _inputAttrs[NSParagraphStyleAttributeName] = _inputParagraphStyle;
    _placeholderAttrs[NSParagraphStyleAttributeName] = _inputParagraphStyle;

    _inputAttrs[NSForegroundColorAttributeName] = [UIColor blackColor];
    _placeholderAttrs[NSForegroundColorAttributeName] = [UIColor colorWithRed:0.235 green:0.263 blue:0.235 alpha:0.3];

    _preValue = @"";
    _keyListener = [[LynxTextKeyListener alloc] init];
  }
  return self;
}

- (void)onFontFaceLoad {
}

LYNX_PROP_SETTER("placeholder-font-size", setPlaceholderFontSize, CGFloat) {
  self.placeholderFontSize = value;
}

- (UIFontWeight)lynxFontWeightToUIFontWeight:(LynxFontWeightType)value {
  if (value == LynxFontWeightNormal) {
    return UIFontWeightRegular;
  } else if (value == LynxFontWeightBold) {
    return UIFontWeightBold;
  } else if (value == LynxFontWeight100) {
    return UIFontWeightUltraLight;
  } else if (value == LynxFontWeight200) {
    return UIFontWeightThin;
  } else if (value == LynxFontWeight300) {
    return UIFontWeightLight;
  } else if (value == LynxFontWeight400) {
    return UIFontWeightRegular;
  } else if (value == LynxFontWeight500) {
    return UIFontWeightMedium;
  } else if (value == LynxFontWeight600) {
    return UIFontWeightSemibold;
  } else if (value == LynxFontWeight700) {
    return UIFontWeightBold;
  } else if (value == LynxFontWeight800) {
    return UIFontWeightHeavy;
  } else if (value == LynxFontWeight900) {
    return UIFontWeightBlack;
  } else {
    return UNDEFINED_FLOAT;
  }
}

LYNX_PROP_SETTER("font-weight", setFontWeight, LynxFontWeightType) {
  UIFontWeight weight = [self lynxFontWeightToUIFontWeight:value];
  if (weight == UNDEFINED_FLOAT) {
    weight = UIFontWeightRegular;
  }
  self.fontWeight = weight;
}

LYNX_PROP_SETTER("placeholder-font-weight", setPlaceholderFontWeight, LynxFontWeightType) {
  self.placeholderFontWeight = [self lynxFontWeightToUIFontWeight:value];
}

LYNX_PROP_SETTER("font-style", setFontStyle, LynxFontStyleType) {
  self.fontStyle = value;
}

LYNX_PROP_SETTER("placeholder-font-style", setPlaceholderFontStyle, LynxFontStyleType) {
  self.placeholderFontStyle = value;
}

LYNX_PROP_SETTER("font-family", setFontFamily, NSString*) {
  self.fontFamily = value;
}

LYNX_PROP_SETTER("placeholder-font-family", setPlaceholderFontFamily, NSString*) {
  self.placeholderFontFamily = value;
}

LYNX_PROP_SETTER("color", setColor, UIColor*) {
  self.inputAttrs[NSForegroundColorAttributeName] = value ? : [UIColor blackColor];
}

LYNX_PROP_SETTER("placeholder-color", setPlaceHolderColor, UIColor*) {
  self.placeholderAttrs[NSForegroundColorAttributeName] = value ? : [UIColor colorWithRed:0.235 green:0.263 blue:0.235 alpha:0.3];
}

LYNX_PROP_SETTER("caret-color", setCaretColor, NSString*) {
  [self.view setTintColor:[LynxColorUtils convertNSStringToUIColor:value]];
}

LYNX_PROP_SETTER("letter-spacing", setLetterSpacing, CGFloat) {
  self.inputAttrs[NSKernAttributeName] = @(value);
  self.placeholderAttrs[NSKernAttributeName] = @(value);
}

LYNX_PROP_SETTER("line-height", setLineHeight, CGFloat) {
  self.inputParagraphStyle.minimumLineHeight = value;
  self.inputParagraphStyle.maximumLineHeight = value;
}

LYNX_PROP_SETTER("line-spacing", setLineSpacing, NSString *) {
  CGFloat spacing = [LynxUnitUtils toPtWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]
                                    unitValue:value
                                 rootFontSize:0
                                  curFontSize:0
                                    rootWidth:0
                                   rootHeight:0
                                withDefaultPt:0];

  self.inputParagraphStyle.lineSpacing = spacing;
}


LYNX_PROP_SETTER("text-align", setTextAlign, LynxTextAlignType) {
  if (value == LynxTextAlignCenter) {
    self.inputParagraphStyle.alignment = NSTextAlignmentCenter;
  } else if (value == LynxTextAlignLeft) {
    self.inputParagraphStyle.alignment = NSTextAlignmentLeft;
  } else if (value == LynxTextAlignRight) {
    self.inputParagraphStyle.alignment = NSTextAlignmentRight;
  } else if (value == LynxTextAlignStart) {
    self.inputParagraphStyle.alignment = NSTextAlignmentNatural;
  }
}

LYNX_PROP_SETTER("direction", setLynxDirection, LynxDirectionType) {
  if (value == LynxDirectionNormal) {
      self.inputAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionNatural|NSWritingDirectionEmbedding]];
      self.placeholderAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionNatural|NSWritingDirectionEmbedding]];
  } else if (value == LynxDirectionLtr) {
    self.inputAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionLeftToRight|NSWritingDirectionEmbedding]];
    self.placeholderAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionLeftToRight|NSWritingDirectionEmbedding]];
  } else if (value == LynxDirectionRtl) {
    self.inputAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionRightToLeft|NSWritingDirectionEmbedding]];
    self.placeholderAttrs[NSWritingDirectionAttributeName] = @[[NSNumber numberWithInt:NSWritingDirectionRightToLeft|NSWritingDirectionEmbedding]];
  }
}


LYNX_PROP_SETTER("confirm-type", setConfirmType, NSString*) {
  NSString *title = value.lowercaseString;
  UIReturnKeyType returnKeyType = UIReturnKeyDefault;
  if ([title isEqualToString:@"send"]) {
    returnKeyType = UIReturnKeySend;
  } else if ([title isEqualToString:@"search"]) {
    returnKeyType = UIReturnKeySearch;
  } else if ([title isEqualToString:@"next"]) {
    returnKeyType = UIReturnKeyNext;
  } else if ([title isEqualToString:@"go"]) {
    returnKeyType = UIReturnKeyGo;
  } else if ([title isEqualToString:@"done"]) {
    returnKeyType = UIReturnKeyDone;
  }
  self.view.returnKeyType = returnKeyType;
}


LYNX_PROP_SETTER("type", setType, NSString*) {
  
  if ([value isEqualToString:@"number"]) {
    [self.view setKeyboardType:UIKeyboardTypeNumberPad];
    self.keyListener = [[LynxDigitKeyListener alloc] initWithParamsNeedsDecimal:YES sign:YES];
  } else if ([value isEqualToString:@"digit"]) {
    [self.view setKeyboardType:UIKeyboardTypeDecimalPad];
    self.keyListener = [[LynxDigitKeyListener alloc] initWithParamsNeedsDecimal:YES sign:NO];
  } else if ([value isEqualToString:@"tel"]) {
    [self.view setKeyboardType:UIKeyboardTypePhonePad];
    self.keyListener = [[LynxDialerKeyListener alloc] init];
  } else if ([value isEqualToString:@"email"]) {
    [self.view setKeyboardType:UIKeyboardTypeEmailAddress];
    self.keyListener = [[LynxTextKeyListener alloc] init];
  } else {
    [self.view setKeyboardType:UIKeyboardTypeDefault];
    self.keyListener = [[LynxTextKeyListener alloc] init];
  }
  if ([value isEqualToString:@"password"]) {
    [self.view setSecureTextEntry:YES];
  } else {
    [self.view setSecureTextEntry:NO];
  }
}

LYNX_PROP_SETTER("ios-auto-correct", setEnableAutoCorrect, BOOL) {
  [self.view setAutocorrectionType:value ? UITextAutocorrectionTypeYes : UITextAutocorrectionTypeNo];
}

LYNX_PROP_SETTER("ios-spell-check", setEnableSpellCheck, BOOL) {
  [self.view setSpellCheckingType:value ? UITextSpellCheckingTypeYes : UITextSpellCheckingTypeNo];
}

LYNX_PROP_SETTER("ios-send-composing-input", setSendComposingInputEvent, BOOL) {
  self.sendComposingInputEvent = value;
}

LYNX_PROP_SETTER("input-filter", setInputFilter, NSString *) {
  self.inputFilterRegex = value;
}

LYNX_PROP_SETTER("maxlength", setMaxLen, int) {
  self.maxLength = value;
}

LYNX_PROP_SETTER("disable", setDisabled, BOOL) {
  [self.view setUserInteractionEnabled:!value];
}

LYNX_PROP_SETTER("readonly", setReadOnly, BOOL) {
  self.readonly = value;
  if (value) {
    [self.view resignFirstResponder];
  }
  [self.view setUserInteractionEnabled:!value];
}

LYNX_PROP_SETTER("placeholder", setPlaceholder, NSString *) {
  if (requestReset) {
      value = nil;
  }
  self.placeholder = value;
}

- (void)propsDidUpdate {
  _font = [[LynxFontFaceManager sharedManager]
                  generateFontWithSize:self.fontSize
                  weight:self.fontWeight
                  style:self.fontStyle
                  fontFamilyName:self.fontFamily
                  fontFaceContext:self.context.fontFaceContext
                  fontFaceObserver:self];
  
  CGFloat placeholderFontSize = self.placeholderFontSize != UNDEFINED_FLOAT ? self.placeholderFontSize : self.fontSize;
  CGFloat placeholderFontWeight = self.placeholderFontWeight != UNDEFINED_FLOAT ? self.placeholderFontWeight : self.fontWeight;
  LynxFontStyleType placeholderFontStyle = self.placeholderFontStyle != UNDEFINED_INT ? self.placeholderFontStyle : self.fontStyle;
  NSString * placeholderFontFamily = self.placeholderFontFamily ? : self.fontFamily;
  
  _placeholderFont = [[LynxFontFaceManager sharedManager]
                      generateFontWithSize:placeholderFontSize
                      weight:placeholderFontWeight
                      style:placeholderFontStyle
                      fontFamilyName:placeholderFontFamily
                      fontFaceContext:self.context.fontFaceContext
                      fontFaceObserver:self];
  
  if (_font) {
    self.inputAttrs[NSFontAttributeName] = _font;
  }
  if (_placeholderFont) {
    self.placeholderAttrs[NSFontAttributeName] = _placeholderFont;;
  }
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent*)event {
  return self.view.userInteractionEnabled && [super shouldHitTest:point withEvent:event];
}


LYNX_UI_METHOD(getValue) {
  callback(kUIMethodSuccess, @{
    @"value" : [self getText],
    @"selectionStart": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.start] : -1),
    @"selectionEnd": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.end] : -1),
    @"isComposing" : @([self.view markedTextRange] != nil),
  });
}


LYNX_UI_METHOD(focus) {
    if ([self.view becomeFirstResponder]) {
        callback(kUIMethodSuccess, nil);
    } else {
        callback(kUIMethodUnknown, @"fail to focus");
    }
}

LYNX_UI_METHOD(blur) {
  if ([self.view isFirstResponder]) {
    if ([self.view resignFirstResponder]) {
      callback(kUIMethodSuccess, nil);
    } else {
      callback(kUIMethodUnknown, @"fail to blur");
    }
  } else {
    callback(kUIMethodSuccess, nil);
  }
}


LYNX_UI_METHOD(setSelectionRange) {
  NSInteger selectionStart = -1;
  NSInteger selectionEnd = -1;
  if ([[params allKeys] containsObject:@"selectionStart"]) {
    selectionStart = [[params objectForKey:@"selectionStart"] intValue];
  }
  if ([[params allKeys] containsObject:@"selectionEnd"]) {
    selectionEnd = [[params objectForKey:@"selectionEnd"] intValue];
  }
  
  NSInteger length = [self getText].length;
  if (selectionStart > length || selectionEnd > length || selectionStart < 0 || selectionEnd < 0) {
    callback(kUIMethodParamInvalid, @"Range does not meet expectations.");
    return;
  }
  UITextPosition* beginning = self.view.beginningOfDocument;
  UITextPosition* start = [self.view positionFromPosition:beginning offset:selectionStart];
  UITextPosition* end = [self.view positionFromPosition:beginning offset:selectionEnd];
  self.view.selectedTextRange = [self.view textRangeFromPosition:start toPosition:end];
  if ([self.view isKindOfClass:UITextView.class]) {
    [self.view scrollRangeToVisible:NSMakeRange(selectionStart, 0)];
  }
  callback(kUIMethodSuccess, nil);
}


- (NSTextAlignment)textAlignment {
  return self.inputParagraphStyle.alignment;
}



- (NSString *)getText {
  return @"";
}

- (BOOL)isComposing {
  return [self.view markedTextRange] != nil;
}


- (BOOL)inputViewShouldReturn:(id<UITextInput>)input {
  
  [self emitEvent:@"confirm" detail:@{
    @"value" : [self getText]
  }];
  
  if (self.view.returnKeyType != UIReturnKeyNext) {
    [self.view resignFirstResponder];
  } else {
    NSInteger nextTag = self.view.tag + 1;
    // Try to find next responder
    UIResponder* nextResponder = [self.context.rootView viewWithTag:nextTag];
    if (nextResponder != nil) {
      // Found next responder, so set it.
      [nextResponder becomeFirstResponder];
    } else {
      [self.view resignFirstResponder];
    }
  }
  return YES;
}


- (BOOL)inputViewDidChange:(id<UITextInput>)input {
  if (![self inputView:input checkInputValidity:[self getText]]) {
    return NO;
  }
  
  [self sendInputEvent];
  
  return YES;
}

- (void)sendInputEvent {
  if (!self.sendComposingInputEvent) {
    NSString *curValue = [self getText];
    if (![curValue isEqualToString:self.preValue]) {
      [self emitEvent:@"input" detail:@{
        @"value" : curValue,
        @"selectionStart": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.start] : -1),
        @"selectionEnd": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.end] : -1),
      }];
      self.preValue = curValue;
    }
  } else {
    NSDictionary *curInputData = @{
      @"value" : [self getText],
      @"selectionStart": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.start] : -1),
      @"selectionEnd": @(self.view.isFirstResponder ? [self.view offsetFromPosition:self.view.beginningOfDocument toPosition:self.view.selectedTextRange.end] : -1),
      @"isComposing" : @([self.view markedTextRange] != nil)
    };
    if (![curInputData isEqual:self.preInputData]) {
      [self emitEvent:@"input" detail:curInputData];
      self.preInputData = curInputData;
    }
  }
}

- (BOOL)inputView:(id<UITextInput>)input shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
  if (!self.readonly) {
      [self emitEvent:@"beforeinput" detail:@{
        @"value" : [self getText],
        @"cursor": @(range.location),
        @"length": @(range.length),
        @"replace" : string ? : @"",
        @"isComposing" : @([self isComposing])
      }];
    NSString *currentText = self.getText;
    NSUInteger newLength = [currentText length] + [string length] - range.length;
    if (newLength > self.maxLength) {
      // MAX
      return NO;
    }
  } else {
    return NO;
  }
  
  return YES;
}

- (void)inputWillBeFilteredFrom:(NSString *)source to:(NSString *)dest {
  // Override by subclass to change text internal
}

- (BOOL)inputView:(id<UITextInput>)input checkInputValidity:(NSString *)source {
  NSString *dest = source;
  dest = [self filterStringByRegex:source];
  dest = [self.keyListener filter:dest start:0 end:dest.length dest:@"" dstart:0 dend:0];
  dest = [self filterString:dest withMaxLength:self.maxLength];

  if (![source isEqualToString:dest]) {
    [self inputWillBeFilteredFrom:source to:dest];
    return NO;
  }
  
  return YES;
}

- (NSString *)filterStringByRegex:(NSString *)source {
  NSString *regexString = self.isComposing ? nil : self.inputFilterRegex;
  if (!regexString.length) {
    return source;
  }
  
  NSMutableString *dest = [NSMutableString string];
  
  NSError *error;
  NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:regexString options:0 error:&error];

  for (int i = (int)[source length] - 1; i >= 0; i--) {
    NSString *c = [source substringWithRange:NSMakeRange(i, 1)];
    if ([regex numberOfMatchesInString:c options:0 range:NSMakeRange(0, 1)] != 0) {
      [dest insertString:c atIndex:0];
    }
  }
  return [dest copy];
}

- (NSString *)filterString:(NSString *)source withMaxLength:(NSInteger)maxLength {
  if (!source.length) {
    return source;
  }
  
  NSMutableString *dest = [NSMutableString string];
  
  [source enumerateSubstringsInRange:[source rangeOfString:source] options:NSStringEnumerationByComposedCharacterSequences usingBlock:^(NSString * _Nullable substring, NSRange substringRange, NSRange enclosingRange, BOOL * _Nonnull stop) {
    if (dest.length + substring.length > maxLength) {
      *stop = YES;
    } else {
      [dest appendString:substring];
    }
  }];
  
  return dest;
}


- (void)inputViewDidBeginEditing:(id<UITextInput>)input {
  [self emitEvent:@"focus" detail:@{
    @"value" : [self getText]
  }];
}

- (void)inputViewDidEndEditing:(id<UITextInput>)input {
  [self emitEvent:@"blur" detail:@{
    @"value" : [self getText]
  }];
}

- (BOOL)inputViewShouldBeginEditing:(id<UITextInput>)input {
  return !self.readonly;
}


- (void)onWillShowKeyboard:(NSNotification *)notification {
  
  CGRect keyboardFrame = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
  
  CGFloat keyboardHeight = keyboardFrame.size.height;
  
  CGFloat safeAreaBottom = 0;
  
  if (@available(iOS 11.0, *)) {
    safeAreaBottom = self.view.safeAreaInsets.bottom;
  }

  [self emitEvent:@"keyboard" detail:@{
    @"show" : @(YES),
    @"keyboardHeight" : @(keyboardHeight),
    @"safeAreaBottom" : @(safeAreaBottom),
  }];
}

- (void)onWillHideKeyboard:(NSNotification *)notification {
  [self emitEvent:@"keyboard" detail:@{
    @"show" : @(NO),
  }];
}

- (void)emitEvent:(NSString*) name detail:(NSDictionary*)detail {
  LynxCustomEvent *eventInfo = [[LynxDetailEvent alloc] initWithName:name
                                                          targetSign:[self sign]
                                                              detail:detail];
  [self.context.eventEmitter dispatchCustomEvent:eventInfo];
}

- (UIView<UITextInput> *)view {
  return [super view];
}

- (void)triggerLayoutIfNeeded {
  if ([[self.context.nodeOwner nodeWithSign:self.sign] needsLayout]) {
    [[self.context.nodeOwner nodeWithSign:self.sign] internalSetNeedsLayoutForce];
  }
}

- (NSAttributedString *)getAttributedString {
  return nil;
}

- (CGSize)adjustViewSize:(CGSize)viewSize {
  return viewSize;
}

- (void)updateUISize {
  LynxUIBaseInputShadowNode *node = (LynxUIBaseInputShadowNode *)[self.context.nodeOwner nodeWithSign:self.sign];
  if ([node isKindOfClass:LynxUIBaseInputShadowNode.class]) {
    CGSize preSize = node.uiSize;
    
    CGFloat width = self.view.bounds.size.width ? : UIScreen.mainScreen.bounds.size.width;
    [self.view sizeThatFits:CGSizeMake(0, 0)];
    CGSize updatedSize = [self.view sizeThatFits:CGSizeMake(width, CGFLOAT_MAX)];
    // While there is no text, the lineSpacing should not be taken into account
    if (![self getText].length) {
      updatedSize.height -= self.inputParagraphStyle.lineSpacing;
    } else if ([self getContentSize]) {
      CGSize contentSize = [[self getContentSize] CGSizeValue];
      if (contentSize.height != 0) {
        updatedSize = contentSize;
      }
    }
    
    // for placeholder
    updatedSize = [self adjustViewSize:updatedSize];
    
    if (!CGSizeEqualToSize(preSize, updatedSize)) {
      node.uiSize = updatedSize;
      [node setNeedsLayout];
    }
  }
}

- (NSValue *)getContentSize {
  return nil;
}

@end
