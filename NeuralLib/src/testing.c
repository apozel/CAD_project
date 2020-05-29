#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "testing.h"
#include "matrix.h"
#include "learning.h"
#include "random.h"
#include "benchmarking.h"


// Normalization of some inputs:
void test_normalize(void)
{
	printf("\n === Test: normalization ===\n\n");

	int input_number = 10, input_size = 2;

	Number **questions = createMatrix(input_number, input_size);

	randomFillMatrix_uniform(questions, input_number, input_size, 10);

	Inputs* inputs = createInputs(input_number, input_size, 0, questions, NULL);

	printMatrix(inputs -> Questions, input_number, input_size);

	Number **mean_stddev_matrix = find_mean_stddev_matrix(inputs);

	normalize(inputs, mean_stddev_matrix);

	printMatrix(inputs -> Questions, input_number, input_size);

	freeMatrix(&mean_stddev_matrix, 2);
	freeInputs(&inputs);
}


// Testing the Box-Muller transform, a normal distribution generator:
void test_Box_Muller(void)
{
	printf("\n === Test: Box Muller ===\n\n");

	int sampling = 100000;

	// Number of bits of RAND_MAX. Should be 32:
	int bits_number = (int) log(RAND_MAX) / log(2) + 2;

	// Smallest positive real generated by rand():
	double smallest_pos_real = pow(2., -bits_number);

	// Greatest positive real generated using the Box-Muller transform with rand():
	double max_reachable_real = sqrt(-2. * log(smallest_pos_real));

	printf("\nRAND_MAX: %d bits.\nmax_reachable_real = %f\n", bits_number, max_reachable_real);

	int max_std_dev = (int) max_reachable_real + 1;
	int len = 2 * max_std_dev;

	// Slices from -7 to +7 standard deviations. Cannot go over that due to finite precision of rand():
	int *slices = (int*) calloc(len, sizeof(int)); 

	Number x1, x2;

	// Filling the array 'slices' with values generated with the Box-Muller transform:
	for (int i = 0; i < sampling; ++i)
	{
		Box_Muller(&x1, &x2);

		++(slices[max_std_dev + (int) floor(x1)]);
		++(slices[max_std_dev + (int) floor(x2)]);

		// N.B: (int) -0.5 = 0, but floor(-0.5) = -1.
	}

	printf("\nBox-Muller transform:\n\n");
	for (int i = 0; i < max_std_dev; ++i)
		printf("%2d   %f\n", i - max_std_dev, (double) slices[i] / (2 * sampling));
	for (int i = max_std_dev; i < len; ++i)
		printf("%2d   %f\n", i + 1 - max_std_dev, (double) slices[i] / (2 * sampling));
	printf("\n");

	free(slices);
}


// Shuffle demo:
void test_shuffle(void)
{
	printf("\n === Test: shuffle ===\n\n");

	int len = 15;
	// int len = 60000;

	Number *array = (Number*) calloc(len, sizeof(Number));

	Number **adress_array = (Number**) calloc(len, sizeof(Number*));

	if (array == NULL || adress_array == NULL)
	{
		printf("\nNot enough memory.\n\n");
		return;
	}

	// Filling both arrays:

	for (int i = 0; i < len; ++i)
	{
		array[i] = i;
		adress_array[i] = array + i;
	}

	printf("adress_array:\n\n");
	demo_print(adress_array, len);

	// Fisher–Yates shuffle:

	printf("\nFisher–Yates shuffle:\n");

	double time_1 = get_time();

	shuffle((void**) adress_array, len);

	double time_2 = get_time();

	printf("\n-> Time elapsed: %.3f s\n\n", time_2 - time_1);

	demo_print(adress_array, len);

	free(adress_array);
	free(array);
}


// 1 layer neural network for the logical gate 'AND':
void test_AND(void)
{
	printf("\n === Learning: AND ===\n\n");

	int input_number = 4;
	int input_size = 2;
	int answer_size = 1;

	// Creating a neural network:

	int max_batch_size = 4;
	int NeuronsNumberArray[] = {1};
	Activation funArray[] = {Sigmoid};

	int layer_number = ARRAYS_COMPARE_LENGTH(NeuronsNumberArray, funArray);

	NeuralNetwork *network = createNetwork(input_size, layer_number, NeuronsNumberArray, funArray, max_batch_size);

	// Setting the learning parameters:

	LearningParameters *params = initLearningParameters();

	params -> Method = FULL_BATCH;
	params -> EpochNumber = 20;
	params -> LearningRate = 1.;
	params -> LearningRateMultiplier = 0.9;

	// Creating some inputs to learn:

	Number **Questions = createMatrix(input_number, input_size);
	Number **Answers = createMatrix(input_number, answer_size);

	for (int i = 0; i < input_number; ++i)
	{
		// 00, 01, 10, 11:
		Questions[i][0] = i / 2;
		Questions[i][1] = i % 2;

		Answers[i][0] = (int) Questions[i][0] & (int) Questions[i][1];
	}

	Inputs *inputs = createInputs(input_number, input_size, answer_size, Questions, Answers);

	// Saving then loading said inputs:

	saveInputs(inputs, "saves/toLearn_AND");

	Inputs *inputs_loaded = loadInputs("saves/toLearn_AND");

	printInputs(inputs_loaded, ALL);

	// Learning the given inputs:

	learn(network, inputs_loaded, params);

	printNetwork(network, ALL);

	validation(network, inputs_loaded, MAX_VALUE);

	// Prediction:

	Number **the_question = createMatrix(1, input_size);

	the_question[0][0] = 0;
	the_question[0][1] = 0;

	Inputs *inputs_prediction = createInputs(1, input_size, answer_size, the_question, NULL);

	prediction(network, inputs_prediction);

	Number confidence_level;

	int binary_answer = findMostProbable(inputs_prediction -> Answers[0], answer_size, &confidence_level);

	printf("Prediction of 0 AND 0:\n\n");
	printf(" -> Answer: %d\n", binary_answer);
	printf(" -> Confidence level: %.2f %%\n", 100. * confidence_level);

	// Saving:

	saveNetwork(network, "saves/test_AND");

	// Loading:

	NeuralNetwork *network_loaded = loadNetwork("saves/test_AND", max_batch_size);

	printNetwork(network_loaded, ALL);

	// Freeing everything:

	freeNetwork(&network_loaded);
	freeInputs(&inputs_prediction);
	freeInputs(&inputs_loaded);
	freeInputs(&inputs);
	freeParameters(&params);
	freeNetwork(&network);
}


// 2 layers neural network for the logical gate 'XOR':
void test_XOR(void)
{
	printf("\n === Learning: XOR ===\n\n");

	int input_number = 4;
	int input_size = 2;
	int answer_size = 1;

	// Creating a neural network:

	int max_batch_size = 4;
	int NeuronsNumberArray[] = {2, 1};
	Activation funArray[] = {Sigmoid, Sigmoid};

	int layer_number = ARRAYS_COMPARE_LENGTH(NeuronsNumberArray, funArray);

	NeuralNetwork *network = createNetwork(input_size, layer_number, NeuronsNumberArray, funArray, max_batch_size);

	// Setting the learning parameters:

	LearningParameters *params = initLearningParameters();

	params -> PrintEstimates = 1;
	// params -> PrintEstimates = 0;
	params -> Method = FULL_BATCH;
	params -> EpochNumber = 100;
	params -> LearningRate = 0.75;
	params -> LearningRateMultiplier = 0.999;

	// Creating some inputs to learn:

	Number **Questions = createMatrix(input_number, input_size);
	Number **Answers = createMatrix(input_number, answer_size);

	for (int i = 0; i < input_number; ++i)
	{
		// 00, 01, 10, 11:
		Questions[i][0] = i / 2;
		Questions[i][1] = i % 2;

		Answers[i][0] = (int) Questions[i][0] ^ (int) Questions[i][1];
	}

	Inputs *inputs = createInputs(input_number, input_size, answer_size, Questions, Answers);

	printInputs(inputs, ALL);

	Number **mean_stddev_matrix = find_mean_stddev_matrix(inputs);

	normalize(inputs, mean_stddev_matrix);

	// Learning the given inputs:

	learn(network, inputs, params);

	printNetwork(network, ALL);

	validation(network, inputs, MAX_CORRECT);

	// Saving:

	saveNetwork(network, "saves/test_XOR");

	// Freeing everything:

	freeMatrix(&mean_stddev_matrix, 2);
	freeInputs(&inputs);
	freeParameters(&params);
	freeNetwork(&network);
}
