#ifndef __ETL_PROFILE_H__
#define __ETL_PROFILE_H__

#if defined(CAPACITY)
    #undef CAPACITY
#endif

//#define ETL_THROW_EXCEPTIONS
#define ETL_LOG_ERRORS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP

#define ETL_TARGET_OS_FREERTOS // Placeholder. Not currently utilised in the ETL
#define ETL_TARGET_DEVICE_ARM  // Placeholder. Not currently utilised in the ETL



#define VC_EXC(text) etl::exception(text, __FILE__, __LINE__)
#define VC_TRY do
#define VC_THROW(e) new(&exc) VC_EXC(e); break
#define VC_CATCH(exc) while(0); if( exc.what() != "None" ) 
#define VC_ASSERT(cond, text) ETL_ASSERT(cond, VC_EXC(text) )

#endif