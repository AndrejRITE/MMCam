#pragma once
#ifndef POSTPROCESSINGALGORITHMS_H
#define POSTPROCESSINGALGORITHMS_H

#include <numeric>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <string>

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

	template <typename T>
	static auto CalculateMean
	(
		const T* data, 
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

	static auto CalculateVerticalFWHM
	(
		const unsigned short* const dataPtr,
		const unsigned int* const horizontalSumPtr,
		size_t imgWidth,
		size_t imgHeight,
		unsigned int* worstHorizontalSum = nullptr,
		unsigned int* bestHorizontalSum = nullptr,
		int* fwhmMiddlePosY = nullptr,
		unsigned int* minValueInVerticalFWHM = nullptr
	) -> int
	{
		if (!dataPtr || !horizontalSumPtr) return -1;
		if (!imgWidth || !imgHeight) return -1;

		if (fwhmMiddlePosY) *fwhmMiddlePosY = -1;

		// Vertical FWHM Data
		{
			int startOf1DDataCut{};
			{
				// Find the maximum value
				auto minmaxElementIter = std::minmax_element(horizontalSumPtr, &horizontalSumPtr[imgWidth]);
				auto minElementIter = minmaxElementIter.first;
				auto maxElementIter = minmaxElementIter.second;

				if (worstHorizontalSum) *worstHorizontalSum = *minElementIter;
				if (bestHorizontalSum) *bestHorizontalSum = *maxElementIter;

				// Best Position and Sum
				auto bestXPositionInSum = static_cast<int>(std::distance(horizontalSumPtr, maxElementIter));
				//if (bestPosX) *bestPosX = bestXPositionInSum;

				if (bestXPositionInSum < 0 || bestXPositionInSum >= imgWidth) return -1;

				startOf1DDataCut = bestXPositionInSum;
			}

			auto verticalDataSlice = std::make_unique<unsigned short[]>(imgHeight);
			for (auto i{ 0 }; i < imgHeight; ++i)
				verticalDataSlice[i] = dataPtr[imgWidth * i + startOf1DDataCut];

			auto minmaxElementIter = std::minmax_element(verticalDataSlice.get(), &verticalDataSlice[imgHeight]);
			auto maxValue = *minmaxElementIter.second;

			// Average Min value
			auto averageMin = 0U;
			{
				// Mean from the left side
				auto leftMean = CalculateMean(verticalDataSlice.get(), imgHeight, imgHeight / 4, true);
				// Mean from the right side
				auto rightMean = CalculateMean(verticalDataSlice.get(), imgHeight, imgHeight / 4, false);

				averageMin = static_cast<unsigned int>((leftMean + rightMean) / 2.0);
				
				if (minValueInVerticalFWHM) *minValueInVerticalFWHM = averageMin;
			}

			// Calculate the half-maximum
			double halfMax = static_cast<double>(maxValue - averageMin) / 2.0 + averageMin;

			int leftIndex = -1, rightIndex = -1;
			// Looking for the left index
			for (auto i = 0; i < imgHeight; ++i)
			{
				if (leftIndex == -1 && verticalDataSlice[i] >= halfMax)
				{
					leftIndex = static_cast<int>(i);
					break;
				}
			}

			if (leftIndex == -1) return -1;

			// Looking for the right index
			for (auto j = imgHeight - 1; j >= leftIndex; --j)
			{
				if (leftIndex != -1 && verticalDataSlice[j] >= halfMax)
				{
					rightIndex = static_cast<int>(j);
					break;
				}
			}

			// Handle edge cases where the full width is not well-defined
			if (leftIndex == -1 || rightIndex == -1 || rightIndex <= leftIndex) return -1;

			if (fwhmMiddlePosY) *fwhmMiddlePosY = leftIndex + (rightIndex - leftIndex) / 2;

			// Compute FWHM
			return (rightIndex - leftIndex);
		}
	}

	static auto CalculateHorizontalFWHM
	(
		const unsigned short* const dataPtr,
		const unsigned int* const verticalSumPtr,
		size_t imgWidth,
		size_t imgHeight,
		unsigned int* worstVerticalSum = nullptr,
		unsigned int* bestVerticalSum = nullptr,
		int* fwhmMiddlePosX = nullptr,
		unsigned int* minValueInHorizontalFWHM = nullptr
	) -> int
	{
		if (!dataPtr || !verticalSumPtr) return -1;
		if (!imgWidth || !imgHeight) return -1;

		if (fwhmMiddlePosX) *fwhmMiddlePosX = -1;

		auto averagingDataLength = 1.0 / 4;

		// Horizontal FWHM Data
		{
			int startOf1DDataCut{};
			{
				// Find the maximum value
				auto minmaxElementIter = std::minmax_element(verticalSumPtr, &verticalSumPtr[imgHeight]);
				auto minElementIter = minmaxElementIter.first;
				auto maxElementIter = minmaxElementIter.second;

				if (worstVerticalSum) *worstVerticalSum = *minElementIter;
				if (bestVerticalSum) *bestVerticalSum = *maxElementIter;

				// Best Position 
				auto bestYPositionInSum = static_cast<int>(std::distance(verticalSumPtr, maxElementIter));

				if (bestYPositionInSum < 0 || bestYPositionInSum >= imgHeight) return -1;

				startOf1DDataCut = bestYPositionInSum;
			}

			auto horizontalDataSlice = std::make_unique<unsigned short[]>(imgWidth);

			memcpy(horizontalDataSlice.get(), &dataPtr[imgWidth * startOf1DDataCut], sizeof(unsigned short) * imgWidth);

			//for (auto i{ 0 }; i < imgWidth; ++i)
			//	horizontalDataSlice[i] = dataPtr[imgWidth * i + startOf1DDataCut];

			auto minmaxElementIter = std::minmax_element(horizontalDataSlice.get(), &horizontalDataSlice[imgWidth]);
			auto maxValue = *minmaxElementIter.second;

			// Average Min value
			auto averageMin = 0U;
			{
				// Mean from the left side
				auto leftMean = CalculateMean(horizontalDataSlice.get(), imgWidth, imgWidth * averagingDataLength, true);
				// Mean from the right side
				auto rightMean = CalculateMean(horizontalDataSlice.get(), imgWidth, imgWidth * averagingDataLength, false);

				averageMin = static_cast<unsigned int>((leftMean + rightMean) / 2.0);
				if (minValueInHorizontalFWHM) *minValueInHorizontalFWHM = averageMin;
			}

			// Calculate the half-maximum
			double halfMax = static_cast<double>(maxValue - averageMin) / 2.0 + averageMin;

			int leftIndex = -1, rightIndex = -1;
			// Looking for the left index
			for (auto i = 0; i < imgWidth; ++i)
			{
				if (leftIndex == -1 && horizontalDataSlice[i] >= halfMax)
				{
					leftIndex = static_cast<int>(i);
					break;
				}
			}

			if (leftIndex == -1) return -1;

			// Looking for the right index
			for (auto j = imgWidth - 1; j >= leftIndex; --j)
			{
				if (leftIndex != -1 && horizontalDataSlice[j] >= halfMax)
				{
					rightIndex = static_cast<int>(j);
					break;
				}
			}

			// Handle edge cases where the full width is not well-defined
			if (leftIndex == -1 || rightIndex == -1 || rightIndex <= leftIndex) return -1;

			if (fwhmMiddlePosX) *fwhmMiddlePosX = leftIndex + (rightIndex - leftIndex) / 2;

			// Compute FWHM
			return (rightIndex - leftIndex);
		}
	}

	static auto CalculateHEWRadius
	(
		const unsigned short* const dataPtr,
		size_t imgWidth,
		size_t imgHeight,
		int spotCenterPosX,
		int spotCenterPosY,
		unsigned int minValueInData
	) -> int
	{
		if (!dataPtr) return -1;
		if (!imgWidth || !imgHeight) return -1;
		if (spotCenterPosX == -1 || spotCenterPosY == -1) return -1;

		auto maxValue = dataPtr[spotCenterPosY * imgWidth + spotCenterPosX];

		auto wholeDataHeight = maxValue - minValueInData;

		auto percentageToInclude = 90; // Max 100%

		auto minimumThreshold = minValueInData + wholeDataHeight * (100.0 - percentageToInclude) / 100.0;

		// Go to the right side
		auto stepsToRightSide = 0;
		auto currentYPositionInData = spotCenterPosY;
		auto currentXPositionInData = spotCenterPosX;
		auto currentValue = maxValue;

		while (currentXPositionInData < imgWidth && currentValue > minimumThreshold)
		{
			currentValue = dataPtr[currentYPositionInData * imgWidth + currentXPositionInData];
			++currentXPositionInData;
			++stepsToRightSide;
		}

		// Go to the left side
		auto stepsToLeftSide = 0;
		currentYPositionInData = spotCenterPosY;
		currentXPositionInData = spotCenterPosX;
		currentValue = maxValue;
		while (currentXPositionInData >= 0 && currentValue > minimumThreshold)
		{
			currentValue = dataPtr[currentYPositionInData * imgWidth + currentXPositionInData];
			--currentXPositionInData;
			++stepsToLeftSide;
		}

		auto horizontalHEW = std::min(stepsToLeftSide, stepsToRightSide);

		// Go Up
		auto stepsUp = 0;
		currentYPositionInData = spotCenterPosY;
		currentXPositionInData = spotCenterPosX;
		currentValue = maxValue;
		while (currentYPositionInData >= 0 && currentValue > minimumThreshold)
		{
			currentValue = dataPtr[currentYPositionInData * imgWidth + currentXPositionInData];
			--currentYPositionInData;
			++stepsUp;
		}

		// Go Down
		auto stepsDown = 0;
		currentYPositionInData = spotCenterPosY;
		currentXPositionInData = spotCenterPosX;
		currentValue = maxValue;
		while (currentYPositionInData < imgHeight && currentValue > minimumThreshold)
		{
			currentValue = dataPtr[currentYPositionInData * imgWidth + currentXPositionInData];
			++currentYPositionInData;
			++stepsDown;
		}

		auto verticalHEW = std::min(stepsUp, stepsDown);

		return std::min(verticalHEW, horizontalHEW);
	}
}
#endif

