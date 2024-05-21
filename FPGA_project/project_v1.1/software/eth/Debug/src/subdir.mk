################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/data_transaction.c \
../src/main.c \
../src/platform_zynq.c \
../src/tcp_client.c \
../src/tcp_server.c 

OBJS += \
./src/data_transaction.o \
./src/main.o \
./src/platform_zynq.o \
./src/tcp_client.o \
./src/tcp_server.o 

C_DEPS += \
./src/data_transaction.d \
./src/main.d \
./src/platform_zynq.d \
./src/tcp_client.d \
./src/tcp_server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM v7 gcc compiler'
	arm-none-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -I../../eth_bsp/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


