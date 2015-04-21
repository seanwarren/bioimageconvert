/*******************************************************************************

Pointers guarded by reference counting
Author: Dima Fedorov Levit <dimin@dimin.net> <http://www.dimin.net/>

History:
  2008-01-01 17:02 - First creation

ver: 1

*******************************************************************************/

#ifndef BIM_XPOINTER_H
#define BIM_XPOINTER_H

#include <limits>
#include <cmath>
#include <map>

namespace bim {

//------------------------------------------------------------------------------------
// template for a reference counting element
// the static member has to be properly initialized for each instantiation:
//   std::map< T, unsigned int > DUniqueReferences<T>::refs;
//------------------------------------------------------------------------------------

template <typename T> 
class UniqueReferences {
public:
  typedef void (*ReferencesExhausted)( const T &v );

  UniqueReferences() { init(); }
  UniqueReferences(const T &v) { init(v); }
  ~UniqueReferences() { removeReference(it_current); }

  inline T value() const { return (*it_current).first; }
  inline unsigned int references() const { return (*it_current).second; }
  inline bool isNull() const { return (it_current == refs.end()); }

  virtual void setValue( const T &v ) { 
    if (!isNull() && v == value()) return; 
    removeReference( it_current ); 
    addReference(v); 
    it_current = refs.find(v); 
  }

  void removeReference( typename std::map<T, unsigned int>::iterator it ) {
    if (it != refs.end() ) {
      if ( (*it).second>1 ) 
        --(*it).second;
      else {
        // at this point, noone is referring to this element
        (*it).second = 0;
        if (onReferencesExhausted != 0) onReferencesExhausted( (*it).first );
        onExhausted( (*it).first );
        refs.erase( it );
      }
    }
  }

  void decreaseReference( const T &v ) {
    typename std::map<T, unsigned int>::iterator it  = refs.find( v );
    removeReference( it );
  }

  static unsigned int findReferences( const T &v ) {
    typename std::map<T, unsigned int>::const_iterator it  = refs.find( v );
    if (it != refs.end() )
      return (*it).second;
    else 
      return 0; 
  }

  static void addReference( const T &v ) {
    typename std::map<T, unsigned int>::iterator it  = refs.find( v );
    if (it == refs.end() )
      refs.insert( std::make_pair(v, 1) );      
    else 
      ++(*it).second; 
  }

  static void removeReference( const T &v ) {
    typename std::map<T, unsigned int>::iterator it  = refs.find( v );

    if (it != refs.end() ) {
      if ( (*it).second>1 ) 
        --(*it).second;
      else {
        // at this point, noone is referring to this element
        refs.erase( it );
      }
    }
  }

public:
  virtual void onExhausted( const T & ) {}

public:
  ReferencesExhausted onReferencesExhausted;

protected:
  static std::map< T, unsigned int > refs;
  typename std::map< T, unsigned int >::iterator it_current;

  void init() { it_current = refs.end(); onReferencesExhausted = 0; }
  void init( const T &v ) { init(); addReference(v); it_current = refs.find(v); }
};

} // namespace bim

#endif // BIM_XPOINTER_H
