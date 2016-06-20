void start_measurement(void);
void end_measurement(void);
void print_float(float);

float pi(float num_its) {
	float res = 0.0;
	float i = 0.0;
	while(i < num_its) {
		res = res + (4.0 /  (1.0 + (i*2.0)));
		res = res - (4.0 /  (1.0 + ((i+1.0)*2.0)));
		i=i+2.0;
	}
	return res;
}

int main() {
	start_measurement();
	float res = pi(2000.0);
	end_measurement();
	print_float(res);
	return 0;
}
