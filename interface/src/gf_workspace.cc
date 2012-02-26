/* -*- c++ -*- (enables emacs c++ mode) */
/*===========================================================================
 
 Copyright (C) 2006-2012 Yves Renard, Julien Pommier.
 
 This file is a part of GETFEM++
 
 Getfem++  is  free software;  you  can  redistribute  it  and/or modify it
 under  the  terms  of the  GNU  Lesser General Public License as published
 by  the  Free Software Foundation;  either version 2.1 of the License,  or
 (at your option) any later version.
 This program  is  distributed  in  the  hope  that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or  FITNESS  FOR  A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License for more details.
 You  should  have received a copy of the GNU Lesser General Public License
 along  with  this program;  if not, write to the Free Software Foundation,
 Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 
===========================================================================*/

#include <getfemint.h>
#include <getfemint_workspace.h>
#include <algorithm>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <iomanip>
#include <getfem/getfem_mat_elem.h>
#include <getfemint_mdbrick.h>

using namespace getfemint;

  class match_workspace {
    id_type wid;
  public:
    match_workspace(id_type wid_) { wid = wid_; }
    bool operator()(const getfem_object *o) {
      return o->get_workspace() == wid;
    }
  };

static void
do_stat(id_type wid) {
  const workspace_stack::obj_ct obj = workspace().get_obj_list();
  const workspace_stack::wrk_ct wrk = workspace().get_wrk_list();
  int cnt = 0;

  if (wid == workspace_stack::anonymous_workspace) {
    infomsg() << "Anonymous workspace (objects waiting for deletion)\n";
  } else {
    if (!wrk.index()[wid]) THROW_INTERNAL_ERROR;
    infomsg() << "Workspace " << wid << " [" << wrk[wid].get_name() << " -- " << 
      std::count_if(obj.tas_begin(), obj.tas_end(), match_workspace(wid)) << " objects]\n";
  }
  for (workspace_stack::obj_ct::const_tas_iterator it = obj.tas_begin(); it != obj.tas_end(); ++it, ++cnt) {
    if (!obj.index()[it.index()]) THROW_INTERNAL_ERROR;
    if (match_workspace(wid)(*it)) {
      std::string subclassname;
      if ((*it)->class_id() == MDBRICK_CLASS_ID)
	subclassname = "(" + dynamic_cast<getfemint_mdbrick*>(*it)->sub_class() + ")";
      infomsg() << " ID" << std::setw(4) << (*it)->get_id() << " " 
		<< std::setw(14) << name_of_getfemint_class_id((*it)->class_id())
	        << std::setw(20) << subclassname 
		<< "   " << std::setw(9) << (*it)->memsize() << " bytes";
      if ((*it)->is_static()) infomsg() << " * "; else infomsg() << "   ";
      if ((*it)->is_const()) infomsg() << "Const"; else infomsg() << "     ";
      const std::vector<id_type>& used_by = (*it)->get_used_by();
      if (used_by.size()) {
	infomsg() << " used by ";
	for (size_type i=0; i < used_by.size(); ++i) infomsg() << " ID" << used_by[i];
      }
      //      if ((*it)->is_anonymous()) mexPrintf("[ano]");
      infomsg() << endl;
    }
  }
}

static void
do_stats() {
  //infomsg() << "Memory used by elementary matrices structures : " << getfem::stored_mat_elem_memsize()/1024 << "KB\n";
  if (std::count_if(workspace().get_obj_list().tas_begin(), 
		    workspace().get_obj_list().tas_end(), 
		    match_workspace(workspace_stack::anonymous_workspace))) {
    do_stat(workspace_stack::anonymous_workspace);
  }
  for (dal::bv_visitor_c wid(workspace().get_wrk_list().index());
       !wid.finished(); ++wid)
    do_stat(id_type(wid));
}

/*@GFDOC
    Getfem workspace management function. 

    Getfem uses its own workspaces in Matlab, independently of the
    matlab workspaces (this is due to some limitations in the memory
    management of matlab objects). By default, all getfem variables
    belong to the root getfem workspace. A function can create its own
    workspace by invoking gf_workspace('push') at its beginning. When
    exiting, this function MUST invoke gf_workspace('pop') (you can
    use matlab exceptions handling to do this cleanly when the
    function exits on an error).

 @*/



// Object for the declaration of a new sub-command.

struct sub_gf_workspace : virtual public dal::static_stored_object {
  int arg_in_min, arg_in_max, arg_out_min, arg_out_max;
  virtual void run(getfemint::mexargs_in& in,
		   getfemint::mexargs_out& out) = 0;
};

typedef boost::intrusive_ptr<sub_gf_workspace> psub_command;

// Function to avoid warning in macro with unused arguments.
template <typename T> static inline void dummy_func(T &) {}

#define sub_command(name, arginmin, arginmax, argoutmin, argoutmax, code) { \
    struct subc : public sub_gf_workspace {				\
      virtual void run(getfemint::mexargs_in& in,			\
		       getfemint::mexargs_out& out)			\
      { dummy_func(in); dummy_func(out); code }				\
    };									\
    psub_command psubc = new subc;					\
    psubc->arg_in_min = arginmin; psubc->arg_in_max = arginmax;		\
    psubc->arg_out_min = argoutmin; psubc->arg_out_max = argoutmax;	\
    subc_tab[cmd_normalize(name)] = psubc;				\
  }                           




void gf_workspace(getfemint::mexargs_in& m_in, getfemint::mexargs_out& m_out) {
  typedef std::map<std::string, psub_command > SUBC_TAB;
  static SUBC_TAB subc_tab;

  if (subc_tab.size() == 0) {

    /*@FUNC ('push')
      Create a new temporary workspace on the workspace stack. @*/
    sub_command
      ("push", 0, 1, 0, 0,
       std::string s = "unnamed";
       if (!in.remaining() == 0) s = in.pop().to_string();
       workspace().push_workspace(s);
       );

    /*@FUNC ('pop',  [,i,j, ...])
      Leave the current workspace, destroying all getfem objects
      belonging to it, except the one listed after 'pop', and the ones
      moved to parent workspace by ::WORKSPACE('keep'). @*/
    sub_command
      ("pop", 0, 256, 0, 0,
       if (workspace().get_current_workspace()
	   != workspace().get_base_workspace()) {
	 while (in.remaining()) {
	   workspace().send_object_to_parent_workspace
	     (in.pop().to_object_id());
	 }    
	 workspace().pop_workspace();
       } else THROW_ERROR("Can't pop main workspace");
       );


    /*@FUNC ('stat')
       Print informations about variables in current workspace. @*/
    sub_command
      ("stat", 0, 0, 0, 0,
       do_stat(workspace().get_current_workspace());
       infomsg() << endl;
       );


    /*@FUNC ('stats')
       Print informations about all getfem variables. @*/
    sub_command
      ("stats", 0, 0, 0, 0,
       do_stats();
       infomsg() << endl;
       );


    /*@FUNC ('keep', i[,j,k...]) 
      prevent the listed variables from being deleted when
      ::WORKSPACE("pop") will be called by moving these variables in the
      parent workspace. @*/
    sub_command
      ("keep", 1, 256, 0, 0,
       while (in.remaining()) {
	 workspace().send_object_to_parent_workspace(in.pop().to_object_id());
       }
       );


    /*@FUNC ('keep all') 
      prevent all variables from being deleted when
      ::WORKSPACE("pop") will be called by moving the variables in the
      parent workspace. @*/
    sub_command
      ("keep all", 0, 0, 0, 0,
       workspace().send_all_objects_to_parent_workspace();
       );

    /*@FUNC ('clear')
      Clear the current workspace. @*/
    sub_command
      ("clear", 0, 0, 0, 0,
       workspace().clear_workspace();
       );

    /*@FUNC ('clear all')
      Clear every workspace, and returns to the main workspace (you
      should not need this command). @*/
    sub_command
      ("clear all", 0, 0, 0, 0,
       while (workspace().get_current_workspace() != workspace().get_base_workspace()) {
	 workspace().pop_workspace();
	 //      mexPrintf("w <- %d\n", workspace().get_current_workspace());
       }
       workspace().clear_workspace();
       );


    /* Unofficial function */
#ifndef _MSC_VER
    sub_command
      ("chdir", 1, 1, 0, 0,
       if (::chdir(in.pop().to_string().c_str())) {}
       );
#endif

    /*@FUNC ('class name', i)
      Return the class name of object i (if I is a mesh handle, it 
      return gfMesh etc..) @*/
    sub_command
      ("class name", 0, 1, 0, 1,
       id_type id;  id_type cid;
       in.pop().to_object_id(&id, &cid);
       out.pop().from_string(name_of_getfemint_class_id(cid));
       );

    /* Unofficial function */
    sub_command
      ("connect", 0, -1, 0, -1,
       GMM_THROW(getfemint_error, "cannot connect: the toolbox was built without rpc support");
       );


    /* Unofficial function */
    sub_command
      ("list static objects", 0, -1, 0, -1,
       dal::list_stored_objects(cout);
       );


    /* Unofficial function */
    sub_command
      ("nb static objects", 0, -1, 0, 1,
       out.pop().from_integer(int(dal::nb_stored_objects()));
       );

  }





  if (m_in.narg() < 1)  THROW_BADARG( "Wrong number of input arguments");

  std::string init_cmd   = m_in.pop().to_string();
  std::string cmd        = cmd_normalize(init_cmd);

  
  SUBC_TAB::iterator it = subc_tab.find(cmd);
  if (it != subc_tab.end()) {
    check_cmd(cmd, it->first.c_str(), m_in, m_out, it->second->arg_in_min,
	      it->second->arg_in_max, it->second->arg_out_min,
	      it->second->arg_out_max);
    it->second->run(m_in, m_out);
  }
  else bad_cmd(init_cmd);

}
