# TMP-memoization

## 1.왜 만들었는가? - C++ TMP 학습 목적

C++ 템플릿 메타프로그래밍(TMP)을 학습 및 실습하기 위해 메모이제이션(memoization) 라이브러리를 직접 설계·구현하였다.
단순 문법 학습이 아니라, 컨셉(concepts), 태그 디스패치, 부분 특수화, 정책 기반 설계를 모두 사용하는 실용적인 예제를 목표로 삼았다.

---

## 2.무엇을 구현했는가? - TMP를 활용한 메모이제이션 라이브러리

C++ TMP를 활용하여 다음과 같은 특징을 가진 범용 메모이제이션 라이브러리를 구현하였다.

1. 임의의 함수를 감싸 캐시 가능한 함수 객체로 만드는 memoization 템플릿 클래스

2. 컨테이너 정책 :
    - ordered 태그 -> std::map 기반 캐시
    - unordered 태그 -> std::unordered_map 기반 캐시

3. 캐시 제한 정책 :
    - unlimited : 무제한 캐시
    - limited<N> : std::size_t 상수 인자만큼의 데이터를 저장하고, 초과 시 LRU(Latest Recently Used) 방식으로 제거
      
4. 인자 튜플을 키로 사용하는 캐시 구조
    - std::tuple<함수 인자...>를 키로 사용

---

## 3.어떤 점이 차별점인가?

TMP를 사용하지 않고 일반적인 형태로 구현한 메모이제이션 기능은 다음과 같은 단점을 가진다.
- 특정 함수 전용의 컨테이너를 사용하여 인자들과 반환값을 저장한다.
- 반환 타입, 인자 타입, 컨테이너 타입 등이 고정되어 재사용성이 떨어진다.
- 캐시 크기 제한, 반환값 타입 제한 등 정책 적용을 위해선 매번 별도의 구현이 필요하다.

반면에 TMP를 활용한 메모이제이션 라이브러리는 설정된 제약을 만족하면 어떤 함수라도 사용할 수 있다.

또한 반환 타입, 인자 타입, 컨테이너 타입 등이 고정되어있지 않아, 범용적인 사용이 가능하다.

이를 통해 함수별로 특수화된 메모이제이션 기능을 구현하지 않아도 되므로 편리하게 사용할 수 있다.

---

해당 TMP 라이브러리는 다음과 같은 설계를 통해 차별점을 같는다.

1. 정책 기반 설계
  - 컨테이너 (ordered / unordered),  캐시 제한을 템플릿 인자와태그 타입으로 분리함.
  - 동일한 메모이제이션 코드를 유지한 채, 정책 조합만 바꿔서 다양한 캐시 전략을 선택할 수 있음.
  - 반환 값에 제약을 걸어두어 부주의한 라이브러리 사용을 억제할 수 있음.
  - 필요한 경우 정책들을 추가하여 확장 가능함.
    
2. C++20 concepts를 통한 타입 제약
  - 정책 제약을 통해 정책에 어긋나는 조합은 컴파일 시점에 차단함.

3. TMP를 활용한 LRU 구현 구조 분리
  - LRU 기능은 캐시 제한 정책을 사용할 때만 필요하므로 해당 정책이 사용될 때만 코드가 생성됨. 이를 통해 불필요한 코드 생성을 줄임.

4. 범용 함수 / 람다 지원
  - 내부적으로 std::function을 사용해, 일반적인 함수,람다,펑터 등 대부분의 호출 가능 객체를 사용할 수 있도록 설계함. 

---

## 4.상세 구현

1. 반환 타입에 대한 제약
    ```cpp
    template <typename R>
    concept deterministic =
	std::regular<R>;
    ```
    메모이제이션을 사용하기 위해선 반환타입이 레귤러 타입이어야한다. 레귤러 타입은 다음과 같은 특징을 가진다.
   
    - 복사 가능
    - 동등 비교 가능
    - 소멸 시 자원 해제
    - 이동 가능
    - 기본 생성 가능
    - 
    이러한 제약을 설정한 이유는, 등록되는 함수를 순수 함수로 제한하기 위함이다.

    순수 함수란 완전성(어떤 입력에 대해서든 항상 같은 출력이 나옴), 부수효과가 없음, 불변성을 만족하는 함수를 말한다.

    메모이제이션의 함수로 쓰기 위해선 완전성, 부수효과 없음을 만족해야 하지만,

    C++에서는 순수 함수를 강제하기 위한 문법이 존재하지 않는다. (constexpr(상수표현식) 함수는 부수효과 없음, 완전성을 강제하지 못한다.)

    강제할 수 없기 때문에, 순수 함수만을 쓰도록 유도하는 방법을 사용해야한다.

    레귤러 타입만 반환하게 함으로써 해당 함수가 부수효과 없이 값 기반의 추론이 가능한 순수 함수처럼 사용되도록 설계적 제약을 부여한다.

    이외에 추가적인 제약이 필요한 경우, deterministic 컨셉에 제약을 추가할 수 있도록 설계했다.

3. 정책 태그 및 컨셉 정의

    정책 태그와 컨셉에 대한 코드이다. 주석을 통해 상세 내용을 설명한다.
    - 컨테이너 정책
    ```cpp
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
    ```
    
    - 캐시 제한 정책
    ```cpp
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
    ```
    sub_container 구조체의 limited<Size> 특수화를 통해 limited 정책일 때만 해당 구조체가 상속되도록 하여 불필요한 코드 확장을 막는다.

    - 메모이제이션 클래스
    ```cpp
    // 기본 템플릿. 미지정 시 ordered, unlimited로 정책이 설정된다.
    template <typename Signature, typename Order = ordered, typename Limit = unlimited>
    class memoization;
    
    // 특수화. 본체 템플릿.
    template <typename R, typename... Args, typename Order, typename Limit>
    // 정책 확인
    // 1.반환값은 레귤러 타입어야한다.
    // 2.컨테이너를 해쉬맵 또는 맵 중 하나를 선택한다.
    // 3.캐싱 제한 여부와 제한 수를 설정한다.
    	requires deterministic<R> && container_policy<Order> && limit_policy<Limit>
    class memoization<R(Args...), Order, Limit>
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
    ```

    - std::tuple 해쉬 함수 구현 특수화
    ```cpp
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
    ```
---
## 5.결과
1. C++ 20 TMP와 Concepts을 활용하여 정책 기반 메모이제이션 라이브러리를 설계, 구현하였다.
2. 컨테이너 정책, 캐시 제한 정책을 컴파일 타임에 조합 가능하도록 일반화했으며, 잘못된 조합은 컴파일 타임에서 차단된다.
3. 처음 설계할 때는 구체화된 코드를 작성하고, 단계적으로 일반화 시켜나가는 과정을 통해 범용적인 코드를 작성할 수 있었다.
   이 과정을 통해 일반화 프로그래밍 설계를 경험하고, 프로그래밍 실력을 높일 수 있었다. 
4. std::tuple 키와 전용 해시 특수화, 상속을 통한 선택적인 코드 구현 등 개발 과정에서 여러 어려움을 겪고 이를 해결하는 과정을 통해 TMP와 정책 기반 패턴에 대해 깊게 이해할 수 있었다.
5. 추후 타입 추론 기능을 추가하여 복잡한 정책 기입을 줄이고 더욱 편리하게 사용할 수 있도록 개선하고자 한다.

---
