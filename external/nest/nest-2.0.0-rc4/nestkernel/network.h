/*
 *  network.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef NETWORK_H
#define NETWORK_H
#include "config.h"
#include <vector>
#include <string>
#include <typeinfo>
#include "nest.h"
#include "model.h"
#include "scheduler.h"
#include "exceptions.h"
#include "proxynode.h"
#include "connection_manager.h"
#include "event.h"
#include "compose.hpp"
#include "dictdatum.h"
#include <ostream>

#include "dirent.h"
#include "errno.h"

#ifdef M_ERROR
#undef M_ERROR
#endif

#ifdef HAVE_MUSIC
#include "music_event_handler.h"
#endif

/**
 * @file network.h
 * Declarations for class Network.
 */
class TokenArray;
class SLIInterpreter;

namespace nest
{
  class Compound;
  class Event;
  class Node;

  /**
   * @defgroup network Network access and administration
   * @brief Network administration and scheduling.
   * This module contains all classes which are involved in the
   * administration of the Network and the scheduling during
   * simulation.
   */

  /**
   * Main administrative interface to the network.
   * Class Network is responsible for
   * -# Administration of Model objects.
   * -# Administration of network Nodes.
   * -# Administration of the simulation time.
   * -# Update and scheduling during simulation.
   * -# Memory cleanup at exit.
   *
   * @see Node
   * @see Model
   * @ingroup user_interface
   * @ingroup network
   */
  
/* BeginDocumentation
Name: kernel - Global properties of the simulation kernel.

Description:
(start here.)

Parameters:
  The following parameters can be set in the status dictionary.

  communicate_allgather    booltype    - Whether to use MPI_Allgather for communication (otherwise use CPEX)
  data_path                stringtype  - A path, where all data is written to (default is the current directory)
  data_prefix              stringtype  - A common prefix for all data files
  dict_miss_is_error       booltype    - Whether missed dictionary entries are treated as errors
  local_num_threads        integertype - The local number of threads (cf. global_num_virt_procs)
  max_delay                doubletype  - The maximum delay in the network
  min_delay                doubletype  - The minimum delay in the network
  ms_per_tic               doubletype  - The number of miliseconds per tic (cf. tics_per_ms, tics_per_step)
  network_size             integertype - The number of nodes in the network
  num_connections          integertype - The number of connections in the network
  num_processes            integertype - The number of MPI processes
  off_grid_spiking         booltype    - Whether to transmit precise spike times in MPI communicatio
  overwrite_files          booltype    - Whether to overwrite existing data files
  print_time               booltype    - Whether to print progress information during the simulation
  resolution               doubletype  - The resolution of the simulation (in ms)
  rng_buffsize             integertype - The buffer size of the random number generators
  tics_per_ms              doubletype  - The number of tics per milisecond (cf. ms_per_tic, tics_per_step)
  tics_per_step            integertype - The number of tics per simulation time step (cf. ms_per_tic, tics_per_ms)
  time                     doubletype  - The current simulation time
  total_num_virtual_procs  integertype - The total number of virtual processes (cf. local_num_threads)
  to_do                    integertype - The number of steps yet to be simulated
  T_max                    doubletype  - The largest representable time value
  T_min                    doubletype  - The smallest representable time value
SeeAlso: Simulate, Node
*/
  
  class Network
  {
    friend class Scheduler;

  public:

    Network(SLIInterpreter&);
    ~Network();

    /**
     * Reset deletes all nodes and reallocates all memory pools for
     * nodes.
     */
    void reset();

    /**
     * Reset number of threads to one, reset device prefix to the
     * empty string and call reset(). 
     */
    void reset_kernel();    
    
    /**
     * Reset the network to the state at T = 0.
     */
    void reset_network();

    /**
     * Register a built-in model for use with the network.
     * Also enters the model in modeldict, lest private_model is true.
     * @param   m     Model object.
     * @param   private_model  If true, model is not entered in modeldict.
     * @return Model ID assigned by network
     * @note The Network calls the Model object's destructor at exit.
     * @see unregister_model, register_user_model
     */
    index register_model(Model& m, bool private_model = false);

    /**
     * Unregister a previously registered model.
     */
    void unregister_model(index m_id);

    /**
     * Try unregistering model prototype.
     * Throws ModelInUseException, if not possible, does not unregister.
     */
    void try_unregister_model(index m_id);

    /**
     * Copy an existing model and register it as a new model.
     * This function allows users to create their own, cloned models.
     * @param old_id The id of the existing model.
     * @param new_name The name of the new model.
     * @retval Index, identifying the new Model object.
     * @see copy_synapse_prototype()
     * @todo Not fully compatible with thread number changes and
     * unregister_model() yet.
     */
    index copy_model(index old_id, std::string new_name);

    /**
     * Register a synapse prototype at the connection manager.
     */
    index register_synapse_prototype(ConnectorModel * cf);

    /**
     * Unregister a synapse prototype at the connection manager.
     * syn_id: id which was obtained when registering (returned by register_synapse_prototype)
     */
    void unregister_synapse_prototype(index syn_id);

    /**
     * Try unregistering synapse prototype. Throws ModelInUseException, if not possible, does not unregister.
     */
    void try_unregister_synapse_prototype(index syn_id);

    /**
     * Copy an existing synapse type.
     * @see copy_model(), ConnectionManager::copy_synapse_prototype()
     */    
    int copy_synapse_prototype(index sc, std::string);

    /**
     * Return the model id for a given model name.
     */
    int get_model_id(const char []) const;

    /**
     * Return the Model for a given ID.
     */
    Model * get_model(index) const;

    /**
     * Add a number of nodes to the network.
     * This function creates n Node objects of Model m and adds them
     * to the Network at the current position.
     * @param m valid Model ID.
     * @param n Number of Nodes to be created. Defaults to 1 if not
     * specified.
     * @throws nest::UnknownModelID
     */
    index add_node(long_t m, long_t n = 1);

    /**
     * Set the state (observable dynamic variables) of a node to model defaults.
     * @see Node::init_state()
     */
    void init_state(index);

    /**
     * Set the independent parameters and state of a node to model defaults.
     * @see Node::init_node()
     */
    void init_node(index);

    /**
     * Return total number of network nodes.
     * The size also includes all Compound objects.
     */
    index size() const;

    /**
     * Connect two nodes. The two nodes are defined by their global IDs.
     * The connection is established on the thread/process that owns the
     * target node.
     * \param s Address of the sending Node.
     * \param r Address of the receiving Node.
     * \param syn The synapse model to use.
     */ 
    void connect(index s, index r, index syn);

    /**
     * Connect two nodes. The two nodes are defined by their global IDs.
     * The connection is established on the thread/process that owns the
     * target node.
     * \param s Address of the sending Node.
     * \param r Address of the receiving Node.
     * \param w Weight of the connection.
     * \param d Delay of the connection (in ms).
     * \param syn The synapse model to use.
     */ 
    void connect(index s, index r, double_t w, double_t d, index syn);

    /**
     * Connect two nodes. The two nodes are defined by their global IDs.
     * The connection is established on the thread/process that owns the
     * target node.
     * \param s Address of the sending Node.
     * \param r Address of the receiving Node.
     * \param d A parameter dictionary for the connection.
     * \param syn The synapse model to use.
     * \returns true if a connection was made, false if operation was terminated 
     *          because source or target was a proxy.
     */ 
    bool connect(index s, index r, DictionaryDatum& d, index syn);

    void compound_connect(Compound &, Compound &, int, index syn);

    void divergent_connect(index s, TokenArray r, TokenArray weights, TokenArray delays, index syn);
    void random_divergent_connect(index s, TokenArray r, index n, TokenArray w, TokenArray d, bool, bool, index syn);
    
    void convergent_connect(TokenArray s, index r, TokenArray weights, TokenArray delays, index syn);
    void random_convergent_connect(TokenArray s, index t, index n, TokenArray w, TokenArray d, bool, bool, index syn);
 
    DictionaryDatum get_connector_defaults(index sc);
    void set_connector_defaults(index sc, DictionaryDatum& d);

    DictionaryDatum get_synapse_status(index gid, index syn, port p, thread tid);
    void set_synapse_status(index gid, index syn, port p, thread tid, DictionaryDatum& d);

    DictionaryDatum get_connector_status(const Node& node, index sc);
    void set_connector_status(Node& node, index sc, thread tid, DictionaryDatum& d);

    ArrayDatum find_connections(DictionaryDatum dict);

    Compound * get_root() const;        ///< return root compound.
    Compound * get_cwn() const;         ///< current working node.

    /**
     * Change current working node. The specified node must
     * exist and be a compound.
     * @throws nest::IllegalOperation Target is no compound.
     */
    void  go_to(index);

    /**
     * Change current working node. The specified node must
     * exist and be a compound.
     * @throws nest::IllegalOperation  Target is no compound.
     * @throws nest::UnknownNode       Target does not exist in the network.
     */
    void  go_to(std::vector<size_t> const &);

    /**
     * Change current working node. The specified node must
     * exist and be a compound.
     * @throws nest::IllegalOperation  Target is no compound.
     * @throws TypeMismatch            Array is not a flat & homogeneous array of integers.
     * @throws nest::UnknownNode       Target does not exist in the network.
     */
    void  go_to(TokenArray);

    void simulate(Time const &);
    /**
     * Resume the simulation after it was terminated.
     */
    void resume();

    /** 
     * Terminate the simulation after the time-slice is finished.
     */
    void terminate();

    /**
     * Return true if NEST will be quit because of an error, false otherwise.
     */
    bool quit_by_error() const;
    
    /**
     * Return the exitcode that would be returned to the calling shell
     * if NEST would quit now.
     */
    int get_exitcode() const;

    void memory_info();

    void print(TokenArray, int);

    /**
     * Standard routine for sending events. This method decides if
     * the event has to be delivered locally or globally. It exists
     * to keep a clean and unitary interface for the event sending
     * mechanism.
     * @note Only specialization for SpikeEvent does remote sending.
     *       Specialized for DSSpikeEvent to avoid that these events
     *       are sent to remote processes. 
     * \see send_local()
     */
    template <class EventT>
    void send(Node& source, EventT& e, const long_t lag = 0);
    
    /**
     * Send event e to all targets of node source on thread t
     */
    void send_local(thread t, Node& source, Event& e);

    /**
     * Send event e directly to its target node. This should be
     * used only where necessary, e.g. if a node wants to reply
     * to a *RequestEvent immediately.
     */
    void send_to_node(Event& e);

    /**
     * Return minimal connection delay.
     */
    delay get_min_delay() const;

    /**
     * Return maximal connection delay.
     */
    delay get_max_delay() const;
 
    /**
     * Get the time at the beginning of the current time slice.
     */
    Time const& get_slice_origin() const;

    /**
     * Get the time at the beginning of the previous time slice.
     */
    Time get_previous_slice_origin() const;

    /**
     * Get the current simulation time.
     * Defined only while no simulation in progress. 
     */
    Time const get_time() const;

    /**
     * Get random number client of a thread.
     * Defaults to thread 0 to allow use in non-threaded
     * context.  One may consider to introduce an additional
     * RNG just for the non-threaded context.
     */
    librandom::RngPtr get_rng(thread thrd = 0) const;

    /**
     * Get global random number client.
     * This grng must be used synchronized from all threads.
     */
    librandom::RngPtr get_grng() const;

    /**
     * Get number of threads.
     * This function returns the total number of threads per process.
     */
    thread get_num_threads() const;

    /**
     * Suggest a VP for a given global node ID
     */
    thread suggest_vp(index) const;

    /**
     * Convert a given VP ID to the corresponding thread ID
     */
    thread vp_to_thread(thread vp) const;

    /**
     * Convert a given thread ID to the corresponding VP ID
     */
    thread thread_to_vp(thread t) const;

    /**
     * Get number of processes.
     */
    thread get_num_processes() const;

    /**
     * Return true, if the given Node is on the local machine
     */
    bool is_local_node(Node*) const;

    /**
     * Return true, if the given VP is on the local machine
     */
    bool is_local_vp(thread) const;

    /**
     * See Scheduler::get_simulated()
     */
    bool get_simulated() const;
    
    /**
     * Return true, if all Nodes are updated.
     */
    bool is_updated() const;

    /**
     * Get reference signal from the network.
     * Node objects can use this function to determine their update
     * state with respect to the remaining network.
     * If the return value of this function is equal to the value of
     * the Node's local updated flag, then the Node has already been
     * update.
     * For example:
     * @code
     *  bool Node::is_updated() const
     *  {
     *    return stat_.test(updated)==net_->update_reference();
     *  }
     * @endcode
     */
    bool update_reference() const; ///< needed to check update state of nodes.

    /**
     * @defgroup net_access Network access
     * Functions to access network nodes.
     */

    /**
     * Return addess array of the specified Node.
     * @param p Pointer to the specified Node.
     * @ingroup net_access
     */
    std::vector<size_t> get_adr(Node const *p) const;

    /**
     * Return addess array of the specified Node.
     * @param i Index of the specified Node.
     *
     * @ingroup net_access
     */
    std::vector<size_t> get_adr(index i) const;

    /**
     * Return pointer of the specified Node.
     * @param a C++ vector with the address array of the Node.
     * @param thr global thread index of the Node.
     *
     * @throws nest::UnknownNode       Target does not exist in the network.
     *
     * @ingroup net_access
     */
    Node* get_node(std::vector<size_t> const &a, thread thr = 0) const;

    /**
     * Return pointer of the specified Node.
     * @param a SLI Array with the address array of the Node.
     * @param thr global thread index of the Node.
     *
     * @throws TypeMismatch            Array is not a flat & homogeneous array of integers.
     * @throws nest::UnknownNode       Target does not exist in the network.
     *
     * @ingroup net_access
     */
    Node* get_node(TokenArray a, thread thr = 0) const;

    /**
     * Return pointer of the specified Node.
     * @param i Index of the specified Node.
     * @param thr global thread index of the Node.
     *
     * @throws nest::UnknownNode       Target does not exist in the network.
     *
     * @ingroup net_access
     */
    Node*  get_node(index, thread thr = 0) const;

    /**
     * Return the Compound that contains the thread siblings.
     * @param i Index of the specified Node.
     *
     * @throws nest::NoThreadSiblingsAvailable     Node does not have thread siblings.
     *
     * @ingroup net_access
     */
    const Compound* get_thread_siblings(index n) const;

    /**
     * Check, if there are instances of a given model.
     * @param i index of the model to check for
     * @return true, if model is instantiated at least once.
     */
    bool model_in_use(index i) const;

    /**
     * The prefix for files written by devices.
     * The prefix must not contain any part of a path.
     * @see get_data_dir(), overwrite_files()
     */
    const std::string& get_data_prefix() const;

    /**
     * The path for files written by devices.
     * It may be the empty string (use current directory).
     * @see get_data_prefix(), overwrite_files()
     */
    const std::string& get_data_path() const;
    
    /**
     * Indicate if existing data files should be overwritten.
     * @return true if existing data files should be overwritten by devices. Default: false.
     */
    bool overwrite_files() const;
    
    /**
     * return current communication style.
     * A result of true means off_grid, false means on_grid communication.
     */
    bool get_off_grid_communication() const;

    /**
     * Set properties of a Node. The specified node must exist.
     * @throws nest::UnknownNode       Target does not exist in the network.
     * @throws nest::UnaccessedDictionaryEntry  Non-proxy target did not read dict entry.
     * @throws TypeMismatch            Array is not a flat & homogeneous array of integers.
     */
    void set_status(index, const DictionaryDatum&);

    /**
     * Get properties of a node. The specified node must exist.
     * @throws nest::UnknownNode       Target does not exist in the network.
     */
    DictionaryDatum get_status(index) const;

    /**
     * Execute a SLI command in the neuron's namespace.
     */
    int execute_sli_protected(DictionaryDatum, Name);

    /**
     * Return a reference to the model dictionary.
     */
    const Dictionary &get_modeldict();
    
    /**
     * Return the synapse dictionary
     */
    const Dictionary &get_synapsedict() const;
    
    /**
     * Recalibrate scheduler clock.
     */
    void calibrate_clock();
    
    /**
     * Return 0 for even, 1 for odd time slices.
     *
     * This is useful for buffers that need to be written alternatingly
     * by time slice. The value is given by Scheduler::get_slice_() % 2.
     * @see read_toggle
     */
    size_t write_toggle() const;

    /**
     * Return 1 - write_toggle().
     *
     * This is useful for buffers that need to be read alternatingly
     * by slice. The value is given by 1-write_toggle().
     * @see write_toggle
     */
    size_t read_toggle() const;  

    /**
     * Does the network contain copies of models created using CopyModel?
     */
    bool has_user_models() const;

    /** Display a message. This function displays a message at a 
     *  specific error level. Messages with an error level above
     *  M_ERROR will be written to std::cerr in addition to
     *  std::cout.
     *  \n
     *  \n
     *  The message will ony be displayed if the current verbosity level
     *  is greater than or equal to the input level.
     *
     *  @ingroup SLIMessaging
     */
    void message(int level, const char from[], const char text[]);
    void message(int level, const std::string& loc, const std::string& msg);

    /**
     * Returns true if unread dictionary items should be treated as error.
     */
    bool dict_miss_is_error() const;

#ifdef HAVE_MUSIC
  public:  
    /**
     * Register a MUSIC input port (portname) with the port list.
     * This will increment the counter of the respective entry in the
     * music_in_portlist.
     */
    void register_music_in_port(std::string portname);

    /**
     * Unregister a MUSIC input port (portname) from the port list.
     * This will decrement the counter of the respective entry in the
     * music_in_portlist and remove the entry if the counter is 0
     * after decrementing it.
     */
    void unregister_music_in_port(std::string portname);

    /**
     * Register a node (of type music_input_proxy) with a given MUSIC
     * port (portname) and a specific channel. The proxy will be
     * notified, if a MUSIC event is being received on the respective
     * channel and port.
     */
    void register_music_event_in_proxy(std::string portname, int channel, nest::Node *mp);

    /**
     * Set the acceptable latency (latency) for a music input port (portname).
     */
    void set_music_in_port_acceptable_latency(std::string portname, double_t latency);

    /**
     * The mapping between MUSIC input ports identified by portname
     * and the corresponding acceptable latency (second component of
     * the pair). The first component of the pair is a counter that is
     * used to track how many music_input_proxies are connected to the
     * port.
     * @see register_music_in_port()
     * @see unregister_music_in_port()
     */
    std::map< std::string, std::pair<size_t, double_t> > music_in_portlist_;
    
    /**
     * The mapping between MUSIC input ports identified by portname
     * and the corresponding MUSIC event handler.
     */
    std::map< std::string, MusicEventHandler > music_in_portmap_;

    /**
     * Publish all MUSIC input ports that were registered using
     * Network::register_music_event_in_proxy().
     */
    void publish_music_in_ports_();

    /**
     * Call update() for each of the registered MUSIC event handlers
     * to deliver all queued events to the target music_in_proxies.
     */
    void update_music_event_handlers_(Time const &, const long_t, const long_t);
#endif
    
  private:
    void connect(Node& s, Node& r, thread t, index syn);
    void connect(Node& s, Node& r, thread t, double_t w, double_t d, index syn);
    void connect(Node& s, Node& r, thread t, DictionaryDatum& d, index syn);
    
    /**
     * Initialize the network data structures.
     * init_() is used by the constructor and by reset().
     * @see reset()
     */
    void init_();
    void destruct_nodes_();
    void clear_models_();
        
    /**
     * Helper function to set properties on single node.
     * @param node to set properties for
     * @param dictionary containing properties
     * @param if true (default), access flags are called before
     *        each call so Node::set_status_()
     * @throws UnaccessedDictionaryEntry
     */
    void set_status_single_node_(Node&, const DictionaryDatum&, bool clear_flags = true);  

    //! Helper function to set device data path and prefix.
    void set_data_path_prefix_(const DictionaryDatum& d);

    Scheduler scheduler_;
    SLIInterpreter &interpreter_;
    ConnectionManager connection_manager_;
    
    Compound *root_;               //!< Root node.
    Compound *current_;            //!< Current working node (for insertion).

    /* BeginDocumentation
       Name: synapsedict - Dictionary containing all synapse models.
       Description:
       'synapsedict info' shows the contents of the dictionary
       FirstVersion: October 2005
       Author: Jochen Martin Eppler
       SeeAlso: info
    */
    Dictionary* synapsedict_;      //!< Dictionary for synapse models.

    /* BeginDocumentation
       Name: modeldict - dictionary containing all devices and models of NEST
       Description:
       'modeldict info' shows the contents of the dictionary
       SeeAlso: info, Device, RecordingDevice, iaf_neuron, subnet
    */
    Dictionary* modeldict_;        //!< Dictionary for models.

    std::string data_path_;        //!< Path for all files written by devices 
    std::string data_prefix_;      //!< Prefix for all files written by devices
    bool        overwrite_files_;  //!< If true, overwrite existing data files. 

    /**
     * The list of clean models. The first component of the pair is a
     * pointer to the actual Model, the second is a flag indicating if
     * the model is private. Private models are not entered into the
     * modeldict.
     */
    std::vector< std::pair<Model *, bool> > pristine_models_;
    std::vector<Model *> models_;  //!< The list of available models

    std::vector<Node *> nodes_; //!< The network as flat list of nodes

    bool dict_miss_is_error_;  //!< whether to throw exception on missed dictionary entries
  };

  inline 
  void Network::terminate()
  {
    scheduler_.terminate();
  }

  inline
  bool Network::quit_by_error() const
  {    
    Token t = interpreter_.baselookup(Name("systemdict"));
    DictionaryDatum systemdict = getValue<DictionaryDatum>(t);
    t = systemdict->lookup(Name("errordict"));
    DictionaryDatum errordict = getValue<DictionaryDatum>(t);
    return getValue<bool>(errordict, "quitbyerror");
  }

  inline
  int Network::get_exitcode() const
  {
    Token t = interpreter_.baselookup(Name("statusdict"));
    DictionaryDatum statusdict = getValue<DictionaryDatum>(t);
    return getValue<long>(statusdict, "exitcode");
  }

  inline
  std::vector<size_t> Network::get_adr(index n) const
  {
    return get_adr(get_node(n));
  }

  inline
  index Network::size() const
  {
    return nodes_.size();
  }

  inline
  void Network::connect(Node& s, Node& r, thread t, index syn)
  {
    connection_manager_.connect(s, r, t, syn);
  }

  inline
  void Network::connect(Node& s, Node& r, thread t, double_t w, double_t d, index syn)
  {
    connection_manager_.connect(s, r, t, w, d, syn);
  }

  inline
  void Network::connect(Node& s, Node& r, thread t, DictionaryDatum& p, index syn)
  {
    connection_manager_.connect(s, r, t, p, syn);
  }

  inline
  DictionaryDatum Network::get_synapse_status(index gid, index syn, port p, thread tid)
  {
    return connection_manager_.get_synapse_status(gid, syn, p, tid);
  }

  inline
  void Network::set_synapse_status(index gid, index syn, port p, thread tid, DictionaryDatum& d)
  {
    connection_manager_.set_synapse_status(gid, syn, p, tid, d);
  }

  inline
  DictionaryDatum Network::get_connector_status(const Node& node, index sc)
  {
    return connection_manager_.get_connector_status(node, sc);
  }

  inline
  void Network::set_connector_status(Node& node, index sc, thread tid, DictionaryDatum& d)
  {
    connection_manager_.set_connector_status(node, sc, tid, d);
  }

  inline
  ArrayDatum Network::find_connections(DictionaryDatum params)
  {
    return connection_manager_.find_connections(params);
  }
  
  inline
  void Network::set_connector_defaults(index sc, DictionaryDatum& d)
  {
    connection_manager_.set_prototype_status(sc, d);
  }

  inline
  DictionaryDatum Network::get_connector_defaults(index sc)
  {
   return connection_manager_.get_prototype_status(sc);
  }
  
  inline
  index Network::register_synapse_prototype(ConnectorModel * cm)
  {
    return connection_manager_.register_synapse_prototype(cm);
  }
  
  inline
  void Network::unregister_synapse_prototype(index syn_id)
  {
    connection_manager_.unregister_synapse_prototype(syn_id);
  }

  inline
  void Network::try_unregister_synapse_prototype(index syn_id)
  {
    connection_manager_.try_unregister_synapse_prototype(syn_id);
  }

  inline
  int Network::copy_synapse_prototype(index sc, std::string name)
  {
    return connection_manager_.copy_synapse_prototype(sc, name);
  }
  
  inline
  Time const & Network::get_slice_origin() const
  {
    return scheduler_.get_slice_origin();
  }

  inline
  Time Network::get_previous_slice_origin() const
  {
    return scheduler_.get_previous_slice_origin();
  }

  inline
  Time const Network::get_time() const
  {
    return scheduler_.get_time();
  }

  inline
  Compound * Network::get_root() const
  {
    return root_;
  }

  inline
  Compound* Network::get_cwn(void) const
  {
    return current_;
  }

  inline
  thread Network::get_num_threads() const
  {
    return scheduler_.get_num_threads();
  }

  inline
  thread Network::get_num_processes() const
  {
    return scheduler_.get_num_processes();
  }

  inline
  bool Network::is_local_node(Node* n) const
  {
    return scheduler_.is_local_node(n);
  }

  inline
  bool Network::is_local_vp(thread t) const
  {
    return scheduler_.is_local_vp(t);
  }

  inline
  int Network::suggest_vp(index gid) const
  {
    return scheduler_.suggest_vp(gid);
  }

  inline
  thread Network::vp_to_thread(thread vp) const
  {
    return scheduler_.vp_to_thread(vp);
  }

  inline
  thread Network::thread_to_vp(thread t) const
  {
    return scheduler_.thread_to_vp(t);
  }

  inline
  bool Network::get_simulated() const
  {
    return scheduler_.get_simulated();
  }

  inline
  bool Network::is_updated() const
  {
    return scheduler_.is_updated();
  }

  inline
  bool Network::update_reference() const
  {
    return scheduler_.update_reference();
  }

  inline
  delay Network::get_min_delay() const
  {
    return scheduler_.get_min_delay();
  }

  inline
  delay Network::get_max_delay() const
  {
    return scheduler_.get_max_delay();
  }
  
  template <class EventT>
  inline
  void Network::send(Node& source, EventT& e, const long_t lag)
  {
    e.set_stamp(get_slice_origin() + Time::step(lag + 1));
    e.set_sender(source);
    thread t = source.get_thread();

    assert(!source.has_proxies());
    send_local(t, source, e);
  }

  template <>
  inline
  void Network::send<SpikeEvent>(Node& source, SpikeEvent& e, const long_t lag)
  {
    e.set_stamp(get_slice_origin() + Time::step(lag + 1));
    e.set_sender(source);
    thread t = source.get_thread();

    if (source.has_proxies())
    {
      if ( source.is_off_grid() )
        scheduler_.send_offgrid_remote(t, e, lag);
      else
        scheduler_.send_remote(t, e, lag);
    }
    else
      send_local(t, source, e);
  }

  template <>
  inline
  void Network::send<DSSpikeEvent>(Node& source, DSSpikeEvent& e, const long_t lag)
  {
    e.set_stamp(get_slice_origin() + Time::step(lag + 1));
    e.set_sender(source);
    thread t = source.get_thread();

    assert(!source.has_proxies());
    send_local(t, source, e);
  } 

  inline
  void Network::send_local(thread t, Node& source, Event& e)
  {
    index sgid = source.get_gid();
    connection_manager_.send(t, sgid, e);
  }

  inline
  void Network::send_to_node(Event& e)
  {
    e();
  }

  inline
  void Network::calibrate_clock() 
  {
    scheduler_.calibrate_clock();
  }

  inline
  size_t Network::write_toggle() const
  {
    return scheduler_.get_slice() % 2;
  } 

  inline
  size_t Network::read_toggle() const
  {
    // define in terms of write_toggle() to ensure consistency
    return 1 - write_toggle();
  } 

  inline
  librandom::RngPtr Network::get_rng(thread t) const
  {
    return scheduler_.get_rng(t);
  }

  inline
  librandom::RngPtr Network::get_grng() const
  {
    return scheduler_.get_grng();
  }

  inline
  Model* Network::get_model(index m) const
  {
    if (m < models_.size() && models_[m] != 0)
      return models_[m];
    else
      throw UnknownModelID(m);

    return 0; // this never happens
  }

  inline
  const std::string& Network::get_data_path() const
  {
    return data_path_;
  }

  inline
  const std::string& Network::get_data_prefix() const
  {
    return data_prefix_;
  }

  inline
  bool Network::overwrite_files() const
  {
    return overwrite_files_;
  }

  inline
  bool Network::get_off_grid_communication() const
  {
    return scheduler_.get_off_grid_communication();
  }
  
  inline
  const Dictionary& Network::get_modeldict()
  {
    assert(modeldict_ != 0);
    return *modeldict_;
  }
  
  inline
  const Dictionary& Network::get_synapsedict() const
  {
    assert(synapsedict_ != 0);
    return *synapsedict_;
  }

  inline
  bool Network::has_user_models() const
  {
    return models_.size() > pristine_models_.size();
  }

  inline
  bool Network::dict_miss_is_error() const
  {
    return dict_miss_is_error_;
  }
  
  typedef lockPTR<Network> NetPtr;

  //!< Functor to compare Models by their name.
  class ModelComp : public std::binary_function<int, int, bool>
  {
    const std::vector<Model*> &models;

    public:
      ModelComp(const vector<Model*> &nmodels) : models(nmodels) {}
      bool operator()(int a, int b)
      {
        return models[a]->get_name() < models[b]->get_name();
      }
  };

} // namespace

#endif