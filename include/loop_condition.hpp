#pragma once


struct loop_condition
{
  std::atomic<uint8_t> mybool;

  loop_condition()
  {
    
  }

  operator bool() const
  {
    return (mybool.load() != 0);
  }

  loop_condition& operator=( const bool& b )
  {
    if( b )
      {
	set_true();
      }
    else
      {
	set_false();
      }
    return *this;
  }
  
  bool is_true()
  {
    return ( mybool.load() != 0 );
  }
  
  bool is_false()
  {
    return ( mybool.load() == 0 );
  }
  
  void set_true()
  {
    mybool = 1;
  }
  
  void set_false()
  {
    mybool = 0;
  }
};
