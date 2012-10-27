#include <iterator>
#include <memory>
using namespace std;

template < class T >
class ordered_forward_list_element {
friend class ordered_forward_list;
friend class ordered_forward_list_iterator;
protected:
    typedef ordered_forward_list_element<T> class_t;
    T value;
    class_t* nextElement;
    
    ordered_forward_list_element(T value = T(), class_t* nextElement = NULL):
                                 value(value), nextElement(nextElement) { }
    ordered_forward_list_element(const class_t& element):
            value(element.value), nextElement(element.nextElement) { }
};

template < class T >
class ordered_forward_list_iterator : public iterator<forward_iterator_tag, T> {
friend class ordered_forward_list;
protected:
    typedef ordered_forward_list_iterator<T> class_t;
    typedef ordered_forward_list_element<T> element_t;
    element_t* element;
    
public:
    ordered_forward_list_iterator(element_t* element = NULL):
                                 element(element) { }
    ordered_forward_list_iterator(const class_t& iterator):
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

template < class T, class Compare = less<T> >
class ordered_forward_list {
//---------------Type Defs-------------------
public:
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef ordered_forward_list_iterator<T> iterator;
    typedef ordered_forward_list_iterator<const T> const_iterator;
    typedef size_t size_type;
    typedef ordered_forward_list<T, Compare> class_t;
    typedef ordered_forward_list_element<T> element_t;
    
//---------------Variables-------------------
protected:
    element_t* firstElement;

//-------------Constructors------------------
public:
    //empty (default)
    explicit ordered_forward_list (): firstElement(NULL) { }
    //copy
    ordered_forward_list (const class_t& fwdlst): firstElement(NULL) {
        *this = fwdlst;
    }
    //size/fill
    explicit ordered_forward_list (size_type n, const value_type& val = value_type()): firstElement(NULL) {
        assign(n, val);
    }

//--------------Destructor-------------------
protected:
    void destruct_internals() {
        if (firstElement != NULL) {
            element_t* curElement = firstElement;
            while (curElement->nextElement != NULL) {
                element_t* tmp = curElement->nextElement;
                delete curElement;
                curElement = tmp;
            }
            delete curElement;
            curElement = NULL;
        }
    }
public:
    ~ordered_forward_list() {
        destruct_internals();
    }
    
//---------------Operators-------------------
    //copy
    class_t& operator = ( const class_t& fwdlst ) {
        destruct_internals();
        if (fwdlst.firstElement != NULL) {
            firstElement = new element_t(&(fwdlst.firstElement));
            element_t* curThisElement = firstElement;
            element_t* curElement = fwdlst.firstElement;
            while (curElement.nextElement != NULL) {
                curElement = curElement->nextElement;
                curThisElement->nextElement = new element_t(&curElement);
                curThisElement = curThisElement->nextElement;
            }
        }
        return *this;
    }

//---------------Iterators-------------------
    iterator begin() noexcept {
        return iterator(firstElement);
    }
    const_iterator cbegin() const noexcept {
        return const_iterator(firstElement);
    }

//----------------Capacity-------------------
    bool empty() const noexcept {
        return firstElement == NULL;
    }

//------------Element access-----------------
    reference front() {
        return firstElement->value;
    }

//---------------Modifiers-------------------
    //range
//    template <class InputIterator>
//    void assign ( InputIterator first, InputIterator last );
    //fill
    void assign( size_type n, const value_type& val ) {
        destruct_internals();
        if (n > 0) {
            firstElement = new element_t(value_type(val));
            element_t* curElement = firstElement;
            for (size_type i = 1; i < n; i++) {
                curElement->nextElement = new element_t(value_type(val));
                curElement = curElement->nextElement;
            }
        }
    }
    //initializer list
    void assign ( initializer_list<value_type> il );
    
    //copy
    void push_front ( const value_type& val ) {
        firstElement = element_T(value_type(val), firstElement);
    }
    void pop_front () {
        element_t* tmp = firstElement;
        firstElement = firstElement->nextElement;
        delete tmp;
    }
    
    iterator insert_after (const_iterator position, const value_type& val) {
        element_t* curElement = position->element;
        element_t* newElement;
        if (curElement != NULL)
            newElement = new element_t(value_type(val), curElement->nextElement);
        else
            newElement = new element_t(value_type(val), NULL);
        position->element = newElement;
    }
    iterator insert_after (const_iterator position, size_type n, const value_type& val) {
        const_iterator curPosition = const_iterator(position);
        for (size_type i = 0; i < n; i++) {
            insert_after(position, val);
            position++;
        }
    }
//    template <class InputIterator>
//    iterator insert_after ( const_iterator position, InputIterator first, InputIterator last );
//    iterator insert_after ( const_iterator position, initializer_list<value_type> il );

//    iterator erase ( const_iterator position );
//    iterator erase ( const_iterator position, const_iterator last );

    void swap(class_t& fwdlst) {
        element_t* tmp = fwdlst->firstElement;
        fwdlst->firstElement = firstElement;
        firstElement = tmp;
    }
    
    void resize ( size_type sz , const value_type& val = value_type()) {
        element_t* curElement = firstElement;
        for (size_type i = 0; i < sz; i++) {
        
        }
    }
    
    void clear() noexcept {
        destruct_internals();
    }
};
