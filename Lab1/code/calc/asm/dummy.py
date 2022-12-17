import random

i = 0

while i < 10:

	operand1 = random.randint(0, 10**22)

	operand2 = random.randint(0, 10**22)

	add_res = operand1 + operand2
	mul_res = operand1 * operand2

	print(str(operand1) + "+" + str(operand2) + "=" + str(add_res))
	print(str(operand1) + "*" + str(operand2) + "=" + str(mul_res))
	print("================================================================================")

	i += 1


