/**
 * @file ShitListener.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitUtility.hpp"

namespace Shit
{
	template <class... Args>
	//class Listener : public std::list<std::weak_ptr<std::function<void(Args...)>>>
	class Listener : public std::list<std::shared_ptr<std::function<void(Args...)>>>
	{
	public:
		Listener() {}
		Listener(const Listener &other) = delete;

		void notify(Args... args)
		{
			for (auto &&it = this->begin(); it != this->end(); ++it)
			{
				(**it)(args...);
				//if (it->expired())
				//	it = this->erase(it);
				//else
				//{
				//	(*(it->lock()))(args...);
				//	++it;
				//}
			}
		}
	};
}