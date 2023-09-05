################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../cpp/src/main.cpp 

OBJS += \
./cpp/src/main.o 

CPP_DEPS += \
./cpp/src/main.d 


# Each subdirectory must supply rules for building sources it contributes
cpp/src/%.o: ../cpp/src/%.cpp cpp/src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m0 -std=gnu++14 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F051x8 -c -I../Core/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-cpp-2f-src

clean-cpp-2f-src:
	-$(RM) ./cpp/src/main.d ./cpp/src/main.o

.PHONY: clean-cpp-2f-src

