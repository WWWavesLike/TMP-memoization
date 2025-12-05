#pragma once
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <list>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace opt_utils {

// 반환값에 대한 제약.
// 반환값은 무조건 레귤러 타입이어야한다.
// 추후 확장을 위해 별도의 컨셉으로 정의하였다.
template <typename R>
concept deterministic =
	std::regular<R>;
/*
 *
 ****************************************************************
 *
 */

// 컨테이너 정책.
// 태그 구조체들로 정책을 나타낸다.
// ordered, unordered는 태그 타입으로 ordered는 std::map 자료구조, unordered는 std::unordered_map을 사용한다.
struct ordered {};
struct unordered {};

// 태그 구조체를 담는 구조체.
// 각각 ordered와 unordered 특수화로 연결된다.
template <typename Tag, typename K, typename V>
struct container_of;

// ordered 특수화.
// std::map으로 연결된다.
template <typename K, typename V>
struct container_of<ordered, K, V> {
	using type = std::map<K, V>;
};

// unordered 특수화.
// std::unordered_map으로 연결된다.
template <typename K, typename V>
struct container_of<unordered, K, V> {
	using type = std::unordered_map<K, V>;
};

template <typename Tag, typename K, typename V>
using container_t = typename container_of<Tag, K, V>::type;

// 정책 태그가 ordered인지, unordered인지 확인하는 컨셉.
// 두 정책 모두 아닌 경우 컴파일 오류.
template <typename P>
concept container_policy = std::same_as<P, ordered> ||
						   std::same_as<P, unordered>;

// 제한 태그, 제한 숫자를 상수 템플릿 인자로 받는다.
template <std::size_t Size>
struct limited {};

// size_t 인자가 아닌 경우 false_type으로 설정된다.
// 이를 이용해 다른 인자가 들어온 경우 오류가 된다.
// same_as<L, limit<size_t>>로 비교하는 건 size_t의 상수 인자값을 알 수가 없으므로 불가능하다.
template <typename T>
struct is_limited : std::false_type {};

// 특수화를 통해 size_t로 연결.
template <std::size_t Size>
struct is_limited<limited<Size>> : std::true_type {};

// 무제한.
struct unlimited {};

template <typename L>
concept limit_policy = std::same_as<L, unlimited> ||
					   is_limited<L>::value;

// unlimited인 경우 해당 구조체를 상속 받음.
template <typename Limit, typename T>
struct sub_container {};

// Limit가 limited<Size>일 때만 특수화.
template <std::size_t Size, typename T>
struct sub_container<limited<Size>, T> {
	using type = typename T::iterator;
	static constexpr std::size_t size = Size;
	// 캐시 순서를 관리하기 위한 목적의 list 자료구조.
	// unordered_map, map은 넣은 순서대로 유지되지 않기 때문에
	// 별도의 자료구조를 통해 LRU 매커니즘을 구현한다.
	std::list<type> insertion_order;
};
/*************************************************/
// 기본 템플릿. 미지정 시 ordered, unlimited로 정책이 설정된다.
template <typename Signature, typename Order = ordered, typename Limit = unlimited>
class memoization;

// 특수화. 본체 템플릿.
template <typename R, typename... Args, typename Order, typename Limit>
// 정책 확인
// 1.반환값은 레귤러 타입이어야한다.
// 2.컨테이너를 해쉬맵 또는 맵 중 하나를 선택한다.
// 3.캐싱 제한 여부와 제한 수를 설정한다.
	requires deterministic<R> && container_policy<Order> && limit_policy<Limit>
class memoization<R(Args...), Order, Limit>
	// sub_container를 상속받는다.
	// unlimited인 경우 빈 구조체 sub_container(기본)를 상속받고,
	// limited인 경우 연결리스트를 가진 sub_container(특수화)를 상속 받는다.
	: public sub_container<Limit, container_t<Order, std::tuple<std::decay_t<Args>...>, R>> {
private:
	// 함수 인자를 tuple로 저장.
	using args_type = std::tuple<std::decay_t<Args>...>;
	// 반환값.
	using return_type = R;
	// 정책에 따른 자료구조 별칭.
	container_t<Order, args_type, return_type> values_map;
	std::function<R(Args...)> func;

public:
	// 함수가 정의되지 않은 상태에서 함수가 호출되는 문제를 방지하기 위해
	// 기본 생성자를 delete한다.
	memoization() = delete;
	template <typename F>
	memoization(F &&f) : func{std::forward<F>(f)} {}
	memoization(const memoization &) = default;
	memoization(memoization &&) noexcept = default;
	memoization &operator=(const memoization &) = default;
	memoization &operator=(memoization &&) noexcept = default;

	R operator()(Args... args) {
		// std::tuple의 별칭.
		args_type key(std::forward<Args>(args)...);
		auto it = values_map.find(key);
		// 캐시 히트한 경우 결과를 바로 꺼내서 반환한다.
		if (it != values_map.end()) {
			// limited인 경우 LRU 정책에 따라 갱신을 위해 참조한 원소를 맨 뒤로 보낸다.
			if constexpr (!std::same_as<Limit, unlimited>) {
				auto node = std::find(this->insertion_order.begin(),
									  this->insertion_order.end(),
									  it);
				if (node != this->insertion_order.end()) {
					this->insertion_order.splice(this->insertion_order.end(), this->insertion_order, node);
				}
			}
			// 결과 반환.
			return it->second;
		}
		// 없는 경우 함수를 실행한다.
		R result = func(std::forward<Args>(args)...);
		// limited인 경우 제한 수를 초과하는지 확인하여 가장 사용한지 오래된 원소를 삭제한다.
		if constexpr (!std::same_as<Limit, unlimited>) {
			if (values_map.size() >= this->size && !values_map.empty() && !this->insertion_order.empty()) {
				values_map.erase(this->insertion_order.front());
				this->insertion_order.pop_front();
			}
			// 새 원소를 value_map과 isertion_order에 삽입한다.
			auto [new_it, bool_value] = values_map.emplace(std::move(key), result);
			this->insertion_order.push_back(new_it);
		} else {
			// unlimited인 경우 바로 삽입한다.
			values_map.emplace(std::move(key), result);
		}
		return result;
	}
};
/*
 타입 추론 테스트 용도.
template <typename T>
struct function_traits; // 일반 템플릿 선언

// 1-1) 함수 포인터
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
	using signature = R(Args...);
};

// 1-2) std::function
template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
	using signature = R(Args...);
};

// 1-3) 멤버 함수 포인터 (functor의 operator() 특수화용)
template <typename c, typename r, typename... args>
struct function_traits<r (c::*)(args...) const> {
	using signature = r(args...);
};
template <typename c, typename r, typename... args>
struct function_traits<r (c::*)(args...)> {
	using signature = r(args...);
};
// 1-4) 그 외 functor & 람다
template <typename F>
struct function_traits {
	using signature = typename function_traits<
		decltype(&std::decay_t<F>::operator())>::signature;
};
*/
/*
팩토리 함수 테스트용 코드
template <typename F>
memoization(F) -> memoization<typename function_traits<F>::signature>;
template <typename F>
auto make_memoization(F &&f) {
	using decay_f = std::decay_t<F>;
	using sig = typename function_traits<decay_f>::signature;

	return memoization<sig>(std::function<sig>(std::forward<F>(f)));
}
*/
} // namespace opt_utils
// 튜플 전용 해쉬 함수 특수화.
// std::tuple은 unordered_map에 들어갈 때 해쉬 함수가 존재하지 않아 넣을 수가 없다.
// 전용 해쉬함수를 추가하여 unordered_map의 키값으로 들어갈 수 있다.
namespace std {
template <typename... Ts>
struct hash<tuple<Ts...>> {
	size_t operator()(tuple<Ts...> const &t) const noexcept {
		size_t seed = 0;
		apply([&](auto const &...elems) {
			((
				 seed ^= std::hash<std::decay_t<decltype(elems)>>{}(elems) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2)),
			 ...);
		},
			  t);

		return seed;
	}
};

} // namespace std
