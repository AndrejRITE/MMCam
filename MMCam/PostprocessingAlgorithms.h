#pragma once
#ifndef POSTPROCESSINGALGORITHMS_H
#define POSTPROCESSINGALGORITHMS_H

#include <numeric>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

namespace PostprocessingAlgorithms
{
	static auto CalculateSumHorizontally
	(
		unsigned short* const dataPtr,
		const int& imageWidth,
		const int& imageHeight,
		unsigned int* const results
	) -> void
	{
		if (!dataPtr || !results) return;
		if (!imageWidth || !imageHeight) return;

		auto calculateSum = []
		(
			const unsigned short* start, 
			const unsigned short* end, 
			int startRow, int rows, 
			unsigned int* results, 
			int imageWidth
			)
			{
				for (auto i = 0; i < rows; ++i)
				{
					const unsigned short* row_start = start + i * imageWidth;
					const unsigned short* row_end = row_start + imageWidth;
					results[startRow + i] = std::accumulate(row_start, row_end, 0u);
				}
			};

		int num_threads = std::max(1u, std::thread::hardware_concurrency());
		int rows_per_thread = imageHeight / num_threads;
		int remaining_rows = imageHeight % num_threads;

		std::vector<std::thread> threads;
		threads.reserve(num_threads);

		for (auto t = 0; t < num_threads; ++t)
		{
			int start_row = t * rows_per_thread;
			int rows_to_process = t < num_threads - 1 ? rows_per_thread : rows_per_thread + remaining_rows;

			const unsigned short* chunk_start = dataPtr + start_row * imageWidth;
			threads.emplace_back
			(
				calculateSum, 
				chunk_start, 
				chunk_start + rows_to_process * imageWidth, 
				start_row, 
				rows_to_process, 
				results, 
				imageWidth
			);
		}

		for (auto& t : threads)
			t.join();
	};

	static auto CalculateSumVertically
	(
		unsigned short* const dataPtr,
		const int& imageWidth,
		const int& imageHeight,
		unsigned int* const results
	) -> void
	{
		if (!dataPtr || !results) return;
		if (!imageWidth || !imageHeight) return;

		auto calculateSum = []
		(
			const unsigned short* dataPtr,
			const int& imgWidth,
			const int& imgHeight,
			int col_index,
			unsigned int* retValue
			)
			{
				unsigned int result{};
				for (auto i{ 0 }; i < imgHeight; ++i)
				{
					auto position = i * imgWidth + col_index;
					result += dataPtr[position];
				}
				*retValue = result;
			};


#ifdef ONE_THREAD_CALCULATION
		// Calculation on the single core
		for (auto i = 0; i < imageWidth; ++i)
			calculateSum(dataPtr, imageWidth, imageHeight, i, &results[i]);
#else
		auto dataSize = imageWidth;
		auto max_threads = std::min(static_cast<int>(std::thread::hardware_concurrency()), dataSize);

		int num_threads = dataSize;
		std::vector<std::thread> threads;
		threads.reserve(max_threads);

		for (auto t = 0; t < max_threads; ++t)
		{
			threads.emplace_back([&, t]() {
				for (auto i = t; i < dataSize; i += max_threads)
				{
					calculateSum(dataPtr, imageWidth, imageHeight, i, &results[i]);
				}
				});
		}

		for (auto& t : threads)
			t.join();
#endif // _DEBUG

	};

	static auto StoreArrayDataWithTabulator
	(
		const unsigned int* const array,
		size_t size,
		const std::string& filename
	) -> bool
	{
		// Check for valid array pointer
		if (!array || size == 0)
		{
			std::cerr << "Invalid array or size." << std::endl;
			return false;
		}

		// Open the output file for writing
		std::ofstream outFile(filename);
		if (!outFile.is_open())
		{
			std::cerr << "Failed to open file for writing." << std::endl;
			return false;
		}

		// Iterate over the array and write each element with a tab separator
		for (size_t i = 0; i < size; ++i)
		{
			outFile << array[i];
			if (i != size - 1)  // No tab after the last element
				outFile << '\t';
		}

		outFile.close();
		if (!outFile.good())
		{
			std::cerr << "Failed to write to file." << std::endl;
			return false;
		}

		std::cout << "Data successfully written to " << filename << std::endl;
		return true;
	}

	static auto CalculateMean
	(
		const unsigned int* data, 
		size_t sizeOfArray, 
		size_t sizeToCompute, 
		bool calculateFromTheLeftSide
	) -> double 
	{
		if (sizeOfArray == 0 || sizeToCompute == 0) return 0.0; // Handle empty input

		double mean = 0.0;
		size_t count = 0;
		
		if (calculateFromTheLeftSide)
		{
			for (auto i = 0; i < sizeToCompute; ++i) 
			{
				++count;
				mean += (data[i] - mean) / count;  // Incremental mean update
			}
		}
		else
		{
			if (sizeToCompute > sizeOfArray) return mean;

			for (auto i = sizeOfArray - 1; i >= sizeOfArray - sizeToCompute; --i) 
			{
				++count;
				mean += (data[i] - mean) / count;  // Incremental mean update
			}
		}

		return mean;
	}

	static auto CalculateFWHM
	(
		const unsigned int* const array,
		size_t size,
		int* bestPos = nullptr,
		unsigned int* bestSum = nullptr,
		int* worstPos = nullptr,
		unsigned int* worstSum = nullptr,
		int* fwhmMiddlePosPX = nullptr
	) -> double
	{

		if (!size) return -1.0;

		// Step 1: Find the maximum value
		auto minmaxElementIter = std::minmax_element(array, &array[size - 1]);
		auto minElementIter = minmaxElementIter.first;
		auto maxElementIter = minmaxElementIter.second;
		// Best Position and Sum
		auto bestPosition = static_cast<int>(std::distance(array, maxElementIter));
		if (bestPos) *bestPos = bestPosition;

		auto maxSum = *maxElementIter;
		if (bestSum) *bestSum = maxSum;

		// Worst Position and Sum
		auto minPos = static_cast<int>(std::distance(array, minElementIter));
		if (worstPos) *worstPos = minPos;

		auto minSum = *minElementIter;
		if (worstSum) *worstSum = minSum;

		// Average Min value
		auto averageMinSum = minSum;
		{
			// Mean from the left side
			auto leftMean = CalculateMean(array, size, size / 4, true);
			// Mean from the right side
			auto rightMean = CalculateMean(array, size, size / 4, false);

			averageMinSum = static_cast<unsigned int>((leftMean + rightMean) / 2.0);
		}

		// Step 2: Calculate the half-maximum
		double halfMax = static_cast<double>(maxSum - averageMinSum) / 2.0 + averageMinSum;

		if (bestPosition < 0 || bestPosition > size - 1) return -1.0;

		// Step 3: Find indices where array crosses the half-maximum
		int leftIndex = -1, rightIndex = -1;

		// Looking for the left index
		for (auto i = 0; i < size; ++i)
		{
			if (leftIndex == -1 && array[i] >= halfMax) 
			{
				leftIndex = static_cast<int>(i);
				break;
			}
		}

#ifdef _DEBUG
		if (leftIndex == -1)
			StoreArrayDataWithTabulator
			(
				array, 
				size, 
				"D:\\Projects\\RIGAKU\\MMCam\\MMCam\\src\\dbg_fld\\data.txt"
			);
#endif // _DEBUG


		if (leftIndex == -1) return -1.0;

		// Looking for the right index
		for (auto j = size - 1; j >= leftIndex; --j)
		{
			if (leftIndex != -1 && array[j] >= halfMax) 
			{
				rightIndex = static_cast<int>(j);
				break;
			}
		}

		// Handle edge cases where the full width is not well-defined
		if (leftIndex == -1 || rightIndex == -1 || rightIndex <= leftIndex) return -1.0;

		if (fwhmMiddlePosPX) *fwhmMiddlePosPX = leftIndex + (rightIndex - leftIndex) / 2;

		// Step 4: Compute FWHM
		return static_cast<double>(rightIndex - leftIndex);
	}
}
#endif

