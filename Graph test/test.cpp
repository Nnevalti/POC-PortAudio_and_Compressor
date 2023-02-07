#include <iostream>
#include <cmath>
#include <vector>
#include <cstring>
#include <fstream>

#define PRINT_DB 1
#define WITH_COMPRESSION 2
#define WITH_KNEE 2

#define THRESHOLD -20.0f
#define KNEE 5.0f
#define RATIO 2.0f

inline float db2lin(float db){ // dB to linear
	return powf(10.0f, 0.05f * db); // 0,05 = 1/20
}

inline float lin2db(float lin){ // linear to dB
	return 20.0f * log10f(lin);
}

// Generate n points between 0 and 1 with equal distance
// int main() {
//     int iterations = 100;
//     float step = 1.0 / (iterations - 1);
//     for (int i = 0; i < iterations; i++) {
//         float x = i * step;
//         std::cout << x << std::endl;
//     }

//     return 0;
// }


// Generate all possible floating point representable numbers on a computer between 0 and 1
int main() {
    for (float i = 0; i <= 1; i += std::numeric_limits<float>::epsilon()) {
        std::cout << i << std::endl;
    }

    return 0;
}

#if WITH_COMPRESSION == 0
// No compression
int main(int argc, char const *argv[])
{
	std::vector<float> input;
	std::vector<float> output;

    int iterations = 1000;
    float step = 1.0 / (iterations - 1);
    for (int i = 0; i < iterations; i++) {
        float level = i * step;
		if (level == 0)
			continue;
		input.push_back(level);

		output.push_back(level);
	}

	std::ofstream myfile;

	if (PRINT_DB) {
		std::transform(output.begin(), output.end(), output.begin(), lin2db);
		std::transform(input.begin(), input.end(), input.begin(), lin2db);
		myfile.open ("graph_no_compression_dB.json");
	} else {
		myfile.open ("graph_no_compression_lin.json");
	}

	myfile << "{" << std::endl;
	myfile << " \"x\": [";
	for (int i = 0; i < input.size(); i++) {
		myfile << input[i] << ( i == input.size() - 1 ? "" : ", ");
	}
	myfile << "]," << std::endl;
	myfile << " \"y\": [";
	for (int i = 0; i < output.size(); i++) {
		myfile << output[i] << ( i == output.size() - 1 ? "" : ", ");
	}
	myfile << "]" << std::endl;
	myfile << "}" << std::endl;

	myfile.close();

	return 0;
}

#elif WITH_KNEE == 0
// Compression with ratio (no knee)
int main(int argc, char const *argv[])
{
	std::vector<float> input;
	std::vector<float> output;

	float threshold_lin = db2lin(THRESHOLD); // convert the threshold to linear

    int iterations = 1000;
    float step = 1.0 / (iterations - 1);
    for (int i = 0; i < iterations; i++) {
        float level = i * step;
		if (level == 0)
			continue;
		input.push_back(level);

		if (level > threshold_lin) 
		{
			output.push_back(threshold_lin + ((level - threshold_lin) * 1/RATIO));
		}
		else 
		{
			output.push_back(level);
		}
	}

	std::ofstream myfile;

	if (PRINT_DB) {
		std::transform(output.begin(), output.end(), output.begin(), lin2db);
		std::transform(input.begin(), input.end(), input.begin(), lin2db);
		myfile.open ("graph_compression_no_knee_dB.json");
	} else {
		myfile.open ("graph_compression_no_knee_lin.json");
	}

	myfile << "{" << std::endl;
	myfile << " \"x\": [";
	for (int i = 0; i < input.size(); i++) {
		myfile << input[i] << ( i == input.size() - 1 ? "" : ", ");
	}
	myfile << "]," << std::endl;
	myfile << " \"y\": [";
	for (int i = 0; i < output.size(); i++) {
		myfile << output[i] << ( i == output.size() - 1 ? "" : ", ");
	}
	myfile << "]" << std::endl;
	myfile << "}" << std::endl;

	myfile.close();

	return 0;
}


#else
// Compression with a knee
// int main(int argc, char const *argv[])
// {
// 	std::vector<float> input;
// 	std::vector<float> output;

// 	float threshold_lin = db2lin(THRESHOLD); // convert the threshold to linear
// 	float kneeWidth = db2lin(KNEE); // convert the knee to linear
// 	float kneeStart = db2lin(THRESHOLD - KNEE/2); // get the knee start in linear
// 	float kneeEnd = db2lin(THRESHOLD + KNEE/2); // get the knee end in linear

//     int iterations = 1000;
//     float step = 1.0 / (iterations - 1);
//     for (int i = 0; i < iterations; i++) {
//         float level_lin = i * step;
// 		if (level_lin == 0)
// 			continue;
// 		input.push_back(level_lin);

// 		if (level_lin < kneeStart) {
// 			output.push_back(level_lin);
// 		}
// 		else if (level_lin <= kneeEnd) {
// 			output.push_back((((1/RATIO) - 1)*pow(level_lin-THRESHOLD + (kneeWidth/2), 2))/(2*kneeWidth));
// 		}
// 		else {
// 			output.push_back(threshold_lin + ((level_lin - threshold_lin) / RATIO));
// 		}

// // 	⎧
// // 	⎪ 𝑥𝐺										𝑥𝐺−𝑇<−𝑊2
// // 	⎪
// // 𝑦𝐺= ⎨ 𝑥𝐺+(((1/𝑅−1)(𝑥𝐺−𝑇+(𝑊/2))²)/2𝑊)		|𝑥𝐺−𝑇|≤𝑊2
// // 	⎪
// // 	⎪ 𝑇+𝑥𝐺−𝑇𝑅								𝑥𝐺−𝑇>𝑊2
// // 	⎩
// 		// if (level_lin <= kneeStart) 
// 		// {
// 		// 	output.push_back(level_lin);
// 		// }
// 		// else if (level_lin <= kneeEnd)
// 		// {
// 		// 	float gainReduction = ((level_lin - kneeStart) / kneeWidth) * (threshold_lin - kneeStart) / RATIO;
// 		// 	output.push_back(level_lin - gainReduction);
// 		// }
// 		// else 
// 		// {
// 		// 	output.push_back(threshold_lin + ((level_lin - threshold_lin) / RATIO));
// 		// }
// 	}

// 	std::ofstream myfile;

// 	if (PRINT_DB) {
// 		std::transform(output.begin(), output.end(), output.begin(), lin2db);
// 		std::transform(input.begin(), input.end(), input.begin(), lin2db);
// 		myfile.open ("graph_compression_with_knee_dB.json");
// 	} else {
// 		myfile.open ("graph_compression_with_knee_lin.json");
// 	}



// 	myfile << "{" << std::endl;
// 	myfile << " \"x\": [";
// 	for (int i = 0; i < input.size(); i++) {
// 		myfile << input[i] << ( i == input.size() - 1 ? "" : ", ");
// 	}
// 	myfile << "]," << std::endl;
// 	myfile << " \"y\": [";
// 	for (int i = 0; i < output.size(); i++) {
// 		myfile << output[i] << ( i == output.size() - 1 ? "" : ", ");
// 	}
// 	myfile << "]" << std::endl;
// 	myfile << "}" << std::endl;

// 	myfile.close();

// 	return 0;
// }
#endif