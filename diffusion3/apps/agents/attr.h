//attr.h

#ifdef NS_DIFFUSION

#ifndef DIFF_ATTR
#define DIFF_ATTR

//for now keeping the attr templates here

NRSimpleAttributeFactory<char *> TargetAttr(NRAttribute::TARGET_KEY, NRAttribute::STRING_TYPE);
NRSimpleAttributeFactory<char *> AppStringAttr(APP_KEY2, NRAttribute::STRING_TYPE);
NRSimpleAttributeFactory<int> AppCounterAttr(APP_KEY3, NRAttribute::INT32_TYPE);
NRSimpleAttributeFactory<int> AppDummyAttr(APP_KEY1, NRAttribute::INT32_TYPE);




#endif //attr
#endif // NS
