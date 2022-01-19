
all:
	g++ -fopenmp --std=c++17 thread_pool.cpp -o pool -lpthread
	@echo "Run thread pool demo with: ./pool pool <num_of_jobs>"
