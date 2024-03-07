################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app/pid.c \
../app/servo_control.c \
../app/servo_pid.c 

COMPILED_SRCS += \
./app/pid.src \
./app/servo_control.src \
./app/servo_pid.src 

C_DEPS += \
./app/pid.d \
./app/servo_control.d \
./app/servo_pid.d 

OBJS += \
./app/pid.o \
./app/servo_control.o \
./app/servo_pid.o 


# Each subdirectory must supply rules for building sources it contributes
app/%.src: ../app/%.c app/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: TASKING C/C++ Compiler'
	cctc -cs --dep-file="$(basename $@).d" --misrac-version=2004 -D__CPU__=tc26xb "-fC:/Users/12625/workspace3.5/zhinengche 3.5/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc26xb -Y0 -N0 -Z0 -o "$@" "$<" && \
	if [ -f "$(basename $@).d" ]; then sed.exe -r  -e 's/\b(.+\.o)\b/app\/\1/g' -e 's/\\/\//g' -e 's/\/\//\//g' -e 's/"//g' -e 's/([a-zA-Z]:\/)/\L\1/g' -e 's/\d32:/@TARGET_DELIMITER@/g; s/\\\d32/@ESCAPED_SPACE@/g; s/\d32/\\\d32/g; s/@ESCAPED_SPACE@/\\\d32/g; s/@TARGET_DELIMITER@/\d32:/g' "$(basename $@).d" > "$(basename $@).d_sed" && cp "$(basename $@).d_sed" "$(basename $@).d" && rm -f "$(basename $@).d_sed" 2>/dev/null; else echo 'No dependency file to process';fi
	@echo 'Finished building: $<'
	@echo ' '

app/%.o: ./app/%.src app/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: TASKING Assembler'
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-app

clean-app:
	-$(RM) ./app/pid.d ./app/pid.o ./app/pid.src ./app/servo_control.d ./app/servo_control.o ./app/servo_control.src ./app/servo_pid.d ./app/servo_pid.o ./app/servo_pid.src

.PHONY: clean-app

