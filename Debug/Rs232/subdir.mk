################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
/home/luca/beaglebone_code/Common/Source/Rs232/rs232.o 

C_SRCS += \
/home/luca/beaglebone_code/Common/Source/Rs232/rs232.c 

OBJS += \
./Rs232/rs232.o 

C_DEPS += \
./Rs232/rs232.d 


# Each subdirectory must supply rules for building sources it contributes
Rs232/rs232.o: /home/luca/beaglebone_code/Common/Source/Rs232/rs232.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabi-gcc -I/home/luca/beaglebone_code/Common/Include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


