
#include <string_manip.hpp>

#include <cstdlib>
#include <string>
#include <vector>
#include <stack>

#include <boost/tokenizer.hpp>


std::string CONCATENATE_STR_ARRAY(const std::vector<std::string>& arr, const std::string& sep)
{
  if(arr.size() > 0 )
    {
      std::string ret=arr[0];
      for(size_t a=1; a<arr.size(); ++a)
	{
	  ret += sep + arr[a];
	}
      return ret;
    }
  else
    {
      return "";
    }
}


std::vector<std::string> tokenize_string(const std::string& src, const std::string& delim, bool include_empty_repeats)
{
  std::vector<std::string> retval;
  boost::char_separator<char> sep( delim.c_str() );
  boost::tokenizer<boost::char_separator<char>> tokens(src, sep);
  for(const auto& t : tokens)
    {
      retval.push_back( t );
    }
  return retval;
}



 



  
























std::string get_canonical_dir_of_fname( const std::string& s, std::string& fnametail )
{
  std::stack<std::string> fnstack;

  
  

  
  
  
  
  bool isglobal=false;

  if( s.size() < 1 )
    {
      fprintf(stderr, "REV: error, filename of string in get_canonical_dir_of_fname is empty... [%s]\n", s.c_str() );
      exit(1);
    }
  
  
  
  if( s[0] == '/' )
    {
      isglobal = true;
    }
  
  
  std::vector<std::string> vect = tokenize_string( s, "/");
  
  
  
  

  
  
  
  

  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  size_t negative_stack_size = 0;
  
  for(size_t x=0; x<vect.size(); ++x)
    {
      if( vect[x].compare("..") == 0 )
	{
	  if( fnstack.size() == 0 )
	    {
	      ++negative_stack_size;
	    }
	  else
	    {
	      fnstack.pop();
	    }
	}
      else if( vect[x].compare(".") == 0)
	{
	  
	}
      else
	{
	  fnstack.push( vect[x] );
	}
      
    }

  fnametail = fnstack.top();
  fnstack.pop(); 
  
  std::vector<std::string> ret( fnstack.size() );
  
  
  
  
  for(size_t x=ret.size(); x>0; --x)
    {
      ret[ x-1 ] = fnstack.top();
      fnstack.pop();
    }

  
  std::string finalstring =  CONCATENATE_STR_ARRAY( ret, "/" );
  
  
  if(isglobal)
    {
      
      
      if( finalstring != "" )
	{
	  finalstring = "/" + finalstring;
	}
    }
  else
    {
      std::string doubledotstr = "";
      for(size_t x=0; x<negative_stack_size; ++x)
	{
	  doubledotstr = doubledotstr + "/..";
	}
      
      if( finalstring != "" )
	{
	  finalstring = "/" + finalstring;
	}
      finalstring = "." + doubledotstr + finalstring;
    }

  
  return finalstring;
    
  
    
}


std::string canonicalize_fname( const std::string& s )
{
  std::string fname;
  std::string dir = get_canonical_dir_of_fname( s, fname );

  std::string finalstring = dir + "/" + fname;
  
  return finalstring;
  
}


bool same_fnames(const std::string& fname1, const std::string& fname2)
{
  std::string s1 = canonicalize_fname( fname1 );
  std::string s2 = canonicalize_fname( fname2 );
  if( s1.compare( s2 ) == 0 )
    {
      return true;
    }
  else
    {
      return false;
    }
}


void replace_old_fnames_with_new( std::vector<std::string>& fvect, const std::string& newfname, const std::vector<size_t> replace_locs )
{
  for(size_t x=0; x<replace_locs.size(); ++x)
    {
      
      fvect[ replace_locs[ x ] ] = newfname;
    }
  
  return;
}


std::vector<size_t> find_matching_files(const std::string& fname, const std::vector<std::string>& fnamevect, std::vector<bool>& marked )
{
  std::vector<size_t> foundvect;
  for(size_t x=0; x<fnamevect.size(); ++x)
    {
      std::string canonical = canonicalize_fname( fnamevect[x] );
      
      
      if( same_fnames( canonical, fname ) == true )
	{
	  
	  if(marked[x] == false)
	    {
	      foundvect.push_back( x );
	      marked[x] = true;
	    }
	  else
	    {
	      fprintf( stderr, "REV: WARNING in find_matching_filenames: RE-CHANGING already MARKED file (i.e. circular renaming?) File: [%s]\n",
		       fname.c_str() );
	    }
	}
      
      
    }
  return foundvect;
}
