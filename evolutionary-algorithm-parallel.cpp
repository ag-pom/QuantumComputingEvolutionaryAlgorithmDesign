#include "evolutionary-algorithm-parallel.h"
#include "quantum-computer.h"
#include "quantum-test-generator.h"

ParallelGenerationData insertInGeneration(ParallelGenerationData pData, vector<int> dna, double score) {
	vector<double>::iterator it = lower_bound(pData.scores.begin(), pData.scores.end(), score);
	int index = distance(pData.scores.begin(), it);
	if (pData.scores.size() == 0) {
		pData.scores.push_back(score);
	} else {
		pData.scores.insert(it, score);
	}
	
	if (pData.generation.size() < 1) {
		pData.generation.push_back(dna);
	} else {
		pData.generation.insert(pData.generation.begin() + index, dna);
	}
	return pData;
}

double reward(vector<int> dna) {
	VectorXcd distance;
	double score = 0;
	int num_bits = 5;
	int input_iter = 100;
	Map<VectorXi> dna_e(dna.data(), dna.size());

	QuantumComputer* quant_comp = new QuantumComputer();
	QuantumTestGenerator* quant_test = new QuantumTestGenerator();
	
	quant_comp -> SetNumQBits(num_bits);
	quant_test ->Init(num_bits);
	int i = 0;
	while (i < input_iter) {
		quant_test -> Next();
		quant_test -> input.normalize();
		quant_comp -> states = quant_test -> input;
		quant_comp -> ApplyAlgorithm(dna_e);
		distance = quant_comp -> states - quant_test -> output;
		score += distance.norm();
		i++;
	}

	delete quant_comp;
	delete quant_test;

	return score;
}

ParallelGenerationData scoreDna(vector<int> dna, ParallelGenerationData pData) {
	double rwrd = reward(dna);
	return insertInGeneration(pData, dna, rwrd);
}	

void EvolutionarySearch::SetDnaLength(int len) {
	dnaLength = len;
}

void EvolutionarySearch::SetMutationRate(int mtr) {
	mutationRate = mtr;
}

void EvolutionarySearch::SetGenerationSize(int genSize) {
	generationSize = genSize;
}

vector<int> EvolutionarySearch::GenerateDna() {
	vector<int> dna;
	for (int i = 0; i < dnaLength; i++) {
		int a = rand() % geneNum;
		dna.push_back(a);
	}
	return dna;
}

void EvolutionarySearch::Init() {
	bestScore = -1E10;
	int i = 0;
	ParallelGenerationData pData;
	while (i < generationSize) {
		vector<int> dna = GenerateDna();
		pData = scoreDna(dna, pData);
		i++;
	}
	generation = pData.generation;
	scores = pData.scores;
}

void EvolutionarySearch::EvaluateGeneration() {
	vector< vector<int> > unorderedGeneration;
	unorderedGeneration = generation;
	generation.clear();
	scores.clear();

	vector<ParallelGenerationData> loopGeneration;

	#pragma omp parallel
	{	
		float id = omp_get_thread_num();
		float total = omp_get_num_threads();
		float factor = generationSize / total;
		ParallelGenerationData pData;
		for (int i = factor * id; i < factor * (id +1); i++) {
			pData = scoreDna(unorderedGeneration[i], pData);
		}
		#pragma omp critical
		{
			loopGeneration.push_back(pData);
		}
	copy(pData.generation.begin(), pData.generation.end(), back_inserter(generation));
	copy(pData.scores.begin(), pData.scores.end(), back_inserter(scores));
	}
}

void EvolutionarySearch::BreedGeneration() {
	for (int i = 0; i < (generationSize / 2); i++) {
		unsigned int j = generationSize - i - 1;
		int a = rand() % generationSize;
		generation[i] = Breed(generation[j], generation[a]);
	}
}

vector<int> EvolutionarySearch::Breed(vector<int> dna_a, vector<int> dna_b) {
	for (unsigned int i = 0; i < dna_a.size(); i++)
	{	
		int a = rand() % 2;
		int b = rand() % INT_MAX;
		if (a == 1) {
			dna_a[i] = dna_b[i];
		}
		if (b < mutationRate * INT_MAX) {
			dna_a[i] = rand() % geneNum;
		}
	}
	return dna_a;
}

void EvolutionarySearch::Evolve(int iter) {
	int i = 0;
	while (i < iter) {
		BreedGeneration();
		EvaluateGeneration();
		PrintGeneration(i);
		i++;
	}
}

void EvolutionarySearch::PrintGeneration(int i) {
	cout << i << '\t' << scores[0] << '\t' << scores[scores.size() - 1] << endl;
}
