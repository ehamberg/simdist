/********************************************************************
 *   		ref_ptr.h
 *   Created on Thu Sep 07 2006 by Boye A. Hoeverstad.
 *   Copyright 2009 Boye A. Hoeverstad
 *
 *   This file is part of Simdist.
 *
 *   Simdist is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Simdist is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   ***************************************************************
 *   
 *   A class wrapping a reference counted pointer to object.
 *   Taken from http://www.davethehat.com/articles/smartp.htm.
 *******************************************************************/

#if !defined(__REF_PTR_H__)
#define __REF_PTR_H__

#include <cassert>
#include <cstdlib>

template <class T>
class ref_ptr
{

  class counted
  {
  public:
    size_t m_nCount;
    T* const m_pT;
  
    counted(T* pT) 
        : m_nCount(0), m_pT(pT) 
    { 
      assert(pT != 0); 
//       std::cerr << "Created counted at " << this << ".\n";
    }

    ~counted()
    { 
//       std::cerr << "Deleting counted at " << this << ".\n";
      assert(m_nCount == 0); 
      delete m_pT; 
    }
  

    size_t
    IncRef()  
    { 
//       std::cerr << "Inc'ing ref of " << this << " to " << m_nCount + 1 << ".\n";
      return ++m_nCount; 
    }
    
    size_t
    FreeRef()
    { 
//       std::cerr << "Dec'ing ref of " << this << " to " << m_nCount - 1 << ".\n";
      assert(m_nCount!=0); 
      return --m_nCount; 
    }

  };

public:
  ref_ptr()
      : m_pObject(0) 
  {
  }

  explicit ref_ptr(T* pT)
  {
    m_pObject = new counted(pT);
    m_pObject->IncRef();
  }

  ~ref_ptr()
  {
//     std::cerr << "Deleting ref_ptr at " << this;
//     if (Null())
//       std::cerr << ". Null.\n";
//     else
//       std::cerr << ", pointing at " << m_pObject << " with ref count " << m_pObject->m_nCount << ".\n";
    UnBind();
  }

  ref_ptr(const ref_ptr<T>& rhs)
  {
    m_pObject = rhs.m_pObject;
    if (!Null())
      m_pObject->IncRef();
  }

  ref_ptr<T>& 
  operator=(const ref_ptr<T>& rhs)
  {
        // Test for assignment to self
    if (&rhs == this) // || rhs.m_pObject == m_pObject)
      return *this;
        // This code was according to the article by Harvey meant to
        // handle assignments to self, but that is not the case:
        // 
        // ref_ptr<int> p; 
        // p = p;
        //
        // The assignment calls UnBind, which sets m_pObject = 0 in
        // both this and rhs, so we need the first test above.
        // Testing for assignment when rhs and this points to the same
        // object is however not necessary.
    if (!rhs.Null())
      rhs.m_pObject->IncRef();
    UnBind();
    m_pObject = rhs.m_pObject;
    return *this;
  }
        
  T* 
  operator->()
  {
    return m_pObject->m_pT;
  }

  const T*
  operator->() const
  {
    return m_pObject->m_pT;
  }
  
  T&
  operator *()
  {
    return *(m_pObject->m_pT);
  }

  const T&
  operator *() const
  {
    return *(m_pObject->m_pT);
  }

  T*
  GetPtr() const throw()
  {
    return (m_pObject->m_pT);
  }


  T&
  GetRef() const throw()
  {
    return *(m_pObject->m_pT);
  }


  bool
  operator==(const ref_ptr<T>& rhs) const
  {
    return m_pObject->m_pT == rhs.m_pObject->m_pT;
  }

  bool
  Null() const 
  {
    return m_pObject == 0;
  };

  void
  SetNull()
  { 
    UnBind(); 
  }

private:
  void UnBind()
  {
    if (!Null() && m_pObject->FreeRef() == 0)       
      delete m_pObject;
    m_pObject = 0;
  }

  counted* m_pObject;

};




#endif
