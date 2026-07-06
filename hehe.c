#include <stdio.h>
void modify_pointer(int **x) {
	int two = 2;
	*x = &two;
}

void modify_copy_pointer(int *x) {
	int two = 2;
	x = &two;
}

int main() {
	int x = 1;
	int *pointer_to_x = &x;

	modify_copy_pointer(pointer_to_x);
	printf("%d\n", *pointer_to_x);

	modify_pointer(&pointer_to_x);
	printf("%d\n", *pointer_to_x);
	return 0;
}
