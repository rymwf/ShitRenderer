/**
 * @file ShitObserver.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <functional>
#include <vector>

namespace Shit
{
	template <class... Args>
	class Observer
	{
	public:
		using handler = std::function<void(Args...)>;

		Observer() {}
		Observer(const Observer &other) = delete;

		void Attach(const handler &f)
		{
			mHandlerList.emplace_back(f);
		}
		void Detach(const handler &f)
		{
			for (auto it = mHandlerList.begin(), end = mHandlerList.end(); it != end; ++it)
			{
				if (*it == f)
				{
					mHandlerList.erase(it);
					break;
				}
			}
		}

		void Notify(Args... args)
		{
			for (auto &&ele : mHandlerList)
				ele(args...);
		}

	private:
		std::vector<handler> mHandlerList;
	};
}