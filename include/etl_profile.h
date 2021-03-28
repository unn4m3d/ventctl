#ifndef __ETL_PROFILE_H__
#define __ETL_PROFILE_H__
#if defined(CAPACITY)
    #undef CAPACITY
#endif

#define ETL_THROW_EXCEPTIONS
#define ETL_LOG_ERRORS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP

#define ETL_TARGET_OS_FREERTOS // Placeholder. Not currently utilised in the ETL
#define ETL_TARGET_DEVICE_ARM  // Placeholder. Not currently utilised in the ETL

#endif