#include <iterator>
#include <memory>
using namespace std;

template < class Key, class T >
class ordered_forward_list_element {
friend class ordered_forward_list_iterator;
protected:
    typedef ordered_forward_list_element<Key, T> class_t;
    Key key;
    T value;
    class_t* nextElement;
    
    ordered_forward_list_element(Key key, T value = T(), class_t* nextElement = NULL):
                                 key(key), value(value), nextElement(nextElement) { }
    ordered_forward_list_element(class_t& element):
            key(element.key), value(element.value), nextElement(element.nextElement) { }
};

template < class Key, class T, class Distance = typename allocator<pair<const Key,T> >::difference_type >
class ordered_forward_list_iterator : public iterator<forward_iterator_tag, T, Distance> {
protected:
    typedef ordered_forward_list_iterator<Key, T, Distance> class_t;
    typedef ordered_forward_list_element<Key, T> element_t;
    element_t* element;
    
public:
    ordered_forward_list_iterator(element_t* element = NULL):
                                 element(element) { }
    ordered_forward_list_iterator(class_t& iterator):
                                element(iterator.element) { }
    
    class_t& operator = (class_t& b) {
        element = b.element;
        return this;
    }
    bool operator == (class_t& b) {
        return (this->element == b.element);
    }
    bool operator != (class_t& b) {
        return !(*this == b);
    }
    T& operator * () {
        return element->value;
    }
    class_t& operator ++ () {     //++class_t
        element = element->nextElement;
        return *this;
    }
    class_t operator ++ (int) {  //class_t++
        class_t tmp(*this);
        element = element->nextElement;
        return tmp;
    }
};

/*
template < class T, class Compare = less<T>, class Alloc = allocator<T> >
class ordered_forward_list {
//---------------Type Defs-------------------
public:
    typedef T value_type;
    typedef Alloc allocator_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef allocator_traits<allocator_type>::pointer pointer;
    typedef allocator_traits<allocator_type>::const_pointer const_pointer;
//    typedef ? iterator;   TODO
//    typedef ? const_iterator;   TODO
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    
//---------------Variables-------------------
protected:
    

//-------------Constructors------------------
public:
    explicit forward_list ( const allocator_type& alloc = allocator_type() );
    forward_list ( const forward_list& fwdlst );
    forward_list ( const forward_list& fwdlst, const allocator_type& alloc );
//    forward_list ( forward_list&& fwdlst );
//    forward_list ( forward_list&& fwdlst, const allocator_type& alloc );
    explicit forward_list ( size_type n );
    explicit forward_list ( size_type n, const value_type& val, const allocator_type& alloc = allocator_type() );
//    template < class InputIterator >
//         forward_list ( InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type() );
    forward_list ( initializer_list<value_type> il, const allocator_type& alloc = allocator_type() );

//---------------Iterators-------------------
    iterator before_begin() noexcept;
    const_iterator before_begin() const noexcept;
};*/
