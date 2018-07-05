#pragma once

#include <memory>
#include <vector>
#include "Byte.hpp"
#include "Alignment.hpp"

namespace bbe {
	class StackAllocatorDestructor {
	private:
		const void* m_data;
		void(*destructor) (const void*);

	public:
		template<typename T>
		explicit StackAllocatorDestructor(const T& data)
			: m_data(std::addressof(data)) {

			destructor = [](const void* lambdaData) {
				auto originalType = static_cast<const T*>(lambdaData);
				originalType->~T();
			};
		}

		void operator()() {
			destructor(m_data);
		}
	};

	class StackAllocatorMarker {
	public:
		byte * m_markerValue;
		size_t m_destructorHandle;

		StackAllocatorMarker(byte* markerValue, size_t destructorHandle)
			: m_markerValue(markerValue), m_destructorHandle(destructorHandle) {
			// do nothing
		}
	};

	template <typename Allocator = std::allocator<byte>>
	class StackAllocator {
	private:
		static constexpr size_t STACK_ALLOC_DEFAULT_SIZE = 1024;
		size_t m_size = 0;

		byte* m_data = nullptr;
		byte* m_head = nullptr;

		Allocator* m_parentAllocator;
		bool m_needsToDeleteParentAllocator = false;

		std::vector<StackAllocatorDestructor> destructors;

		template<typename T>
		inline typename std::enable_if<std::is_trivially_destructible<T>::value>::type addDestructorToList(T* object) {
			// do nothing
		}

		template<typename T>
		inline typename std::enable_if<!std::is_trivially_destructible<T>::value>::type addDestructorToList(T* object) {
			destructors.push_back(StackAllocatorDestructor(*object));
		}

	public:
		explicit StackAllocator(size_t size = STACK_ALLOC_DEFAULT_SIZE, Allocator* parentAllocator = nullptr)
			: m_size(size), m_parentAllocator(parentAllocator) {

			if (m_parentAllocator == nullptr) {
				m_parentAllocator = new Allocator;
				m_needsToDeleteParentAllocator = true;
			}

			m_data = m_parentAllocator->allocate(m_size);
			m_head = m_data;
		}

		StackAllocator(const StackAllocator& other)			   = delete; // Copy Constructor
		StackAllocator(StackAllocator&& other)				   = delete; // Move Constructor
		StackAllocator& operator=(const StackAllocator& other) = delete; // Copy Assignment
		StackAllocator& operator=(StackAllocator&& other)	   = delete; // Move Assignment

		~StackAllocator() {
			if (m_data != m_head) {
				__debugbreak();
			}

			m_parentAllocator->deallocate(m_data, m_size);

			if (m_needsToDeleteParentAllocator) {
				delete m_parentAllocator;
			}

			m_data = nullptr;
			m_head = nullptr;
		}

		void* allocate(size_t amountOfBytes, size_t alignment = 1) {
			byte* allocationLocation = (byte*)nextMultiple(alignment, (size_t)m_head);
			byte* newHeadPointer = allocationLocation + amountOfBytes;

			if (newHeadPointer <= std::addressof(m_data[m_size - 1])) {
				m_head = newHeadPointer;
				return allocationLocation;
			}
			else {
				// Stack Allocator Overflow, add error handling
				__debugbreak();
				return nullptr;
			}
		}

		template<typename T, typename... arguments>
		T* allocateObject(size_t amountOfObjects = 1, arguments&&... args) {
			byte* allocationLocation = (byte*)nextMultiple(alignof(T), (size_t)m_head);
			byte* newHeadPointer = allocationLocation + amountOfObjects * sizeof(T);

			if (newHeadPointer <= std::addressof(m_data[m_size - 1])) {
				T* returnPointer = reinterpret_cast<T*>(allocationLocation);
				m_head = newHeadPointer;

				for (size_t i = 0; i < amountOfObjects; i++) {
					T* object = new (std::addressof(returnPointer[i])) T(std::forward<arguments>(args)...);
					addDestructorToList(object);
				}
				return returnPointer;
			}
			else {
				// Stack Allocator Overflow, add error handling
				__debugbreak();
				return nullptr;
			}
		}

		StackAllocatorMarker getMarker() {
			return StackAllocatorMarker(m_head, destructors.size());
		}

		void deallocateToMarker(StackAllocatorMarker sam) {
			m_head = sam.m_markerValue;

			while (destructors.size() > sam.m_destructorHandle) {
				destructors.back()();
				destructors.pop_back();
			}
		}

		void deallocateAll() {
			m_head = m_data;

			while (destructors.size() > 0) {
				destructors.back()();
				destructors.pop_back();
			}
		}
	};
}