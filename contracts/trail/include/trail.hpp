/**
 * Trail is an EOSIO-based voting service that allows users to create ballots that
 * are voted on by a network of registered voters. Trail also offers custom token
 * features that let any user to create their own voting token and configure settings 
 * to match a wide variety of intended use cases. 
 * 
 * @author Craig Branscom
 * @contract trailservice
 * @copyright
 */

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/dispatcher.hpp>
#include <string>
#include <algorithm>
#include <eosiolib/print.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("trail")]] trail : public contract {
    
    public:

    trail(name self, name code, datastream<const char*> ds);

    ~trail();


    //system defs

    struct user_resources {
        name owner;
        asset net_weight;
        asset cpu_weight;
        int64_t ram_bytes = 0;

        uint64_t primary_key()const { return owner.value; }
        EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
    };

    typedef eosio::multi_index<name("userres"), user_resources> user_resources_table;


    //definitions

    const symbol VOTE_SYM = symbol("VOTE", 0);
    const uint32_t MIN_BALLOT_LENGTH = 86400; //1 day
    const uint32_t MIN_CLOSE_LENGTH = 259200; //3 days
    const uint16_t MAX_VOTE_RECEIPTS = 51; //TODO: move to token registry?

    enum ballot_status : uint8_t {
        SETUP, //0
        OPEN, //1
        CLOSED, //2 //TODO: need closed status? anything after open is closed
        ARCHIVED, //3
        RESERVED_STATUS //4 //RESOLVING?
    };

    struct option {
        name option_name;
        string info; //TODO: remove? ballot info could explain each option
        asset votes;
    };

    struct token_settings {
        bool is_destructible = false;
        bool is_proxyable = false;
        bool is_burnable = false;
        bool is_seizable = false;
        bool is_max_mutable = false;
        bool is_transferable = false;
    };



    //@scope get_self().value
    TABLE ballot {
        name ballot_name;
        name category;
        name publisher;

        string title;
        string description;
        string info_url;

        vector<option> options;
        uint32_t unique_voters;
        uint8_t max_votable_options;
        symbol voting_symbol;

        uint32_t begin_time;
        uint32_t end_time;
        uint8_t status;

        uint64_t primary_key() const { return ballot_name.value; }
        EOSLIB_SERIALIZE(ballot, (ballot_name)(category)(publisher)
            (title)(description)(info_url)
            (options)(unique_voters)(max_votable_options)(voting_symbol)
            (begin_time)(end_time)(status))
    };

    //@scope get_self().value
    TABLE registry {
        asset supply;
        asset max_supply;
        name publisher;
        uint32_t total_voters;
        uint32_t total_proxies;
        token_settings settings;
        string info_url;

        //TODO: maybe track max_vote_receipts here? or maybe in token_settings?

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
        EOSLIB_SERIALIZE(registry, (supply)(max_supply)(publisher)
            (total_voters)(total_proxies)(settings)(info_url))
    };

    //@scope name.value
    TABLE vote {
        name ballot_name;
        vector<name> option_names;
        asset amount;
        uint32_t expiration; //TODO: keep? could pull from ballot end time, removes ability to index by exp though

        uint64_t primary_key() const { return ballot_name.value; }
        uint64_t by_exp() const { return static_cast<uint64_t>(expiration); }
        EOSLIB_SERIALIZE(vote, (ballot_name)(option_names)(amount)(expiration))
    };

    //@scope name.value
    TABLE account {
        asset balance;
        uint16_t num_votes;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance)(num_votes))
    };



    //table defs

    typedef multi_index<name("ballots"), ballot> ballots;

    typedef multi_index<name("registries"), registry> registries;

    typedef multi_index<name("accounts"), account> accounts;

    typedef multi_index<name("votes"), vote,
        indexed_by<name("byexp"), const_mem_fun<vote, uint64_t, &vote::by_exp>>> votes;



    //actions

    ACTION newballot(name ballot_name, name category, name publisher, 
        string title, string description, string info_url, uint8_t max_votable_options,
        symbol voting_sym);

    //TODO: add max_votable_options to params
    ACTION setinfo(name ballot_name, name publisher,
        string title, string description, string info_url);

    ACTION addoption(name ballot_name, name publisher,
        name option_name, string option_info);

    //ACTION rmvoption(name ballot_name, name publisher, name option_name);

    ACTION readyballot(name ballot_name, name publisher, uint32_t end_time);

    ACTION closeballot(name ballot_name, name publisher, uint8_t new_status);

    ACTION deleteballot(name ballot_name, name publisher);

    ACTION castvote(name voter, name ballot_name, name option);

    ACTION unvote(name voter, name ballot_name, name option);

    ACTION rebalance(name voter); //TODO: after unstake, require rebalancing before voting again?

    ACTION cleanupvotes(name voter, uint16_t count, symbol voting_sym);

    ACTION cleanhouse(name voter, symbol voting_sym);

    //ACTION archive(name ballot_name); //TODO: change to reqarchive()? //TODO: require a TLOS transfer to archive?



    ACTION newtoken(name publisher, asset max_supply, token_settings settings, string info_url);

    ACTION mint(name publisher, name recipient, asset amount_to_mint);

    ACTION burn(name publisher, asset amount_to_burn);

    ACTION send(name sender, name recipient, asset amount, string memo);

    ACTION seize(name publisher, name owner, asset amount_to_seize);

    ACTION open(name owner, symbol token_sym);

    ACTION close(name owner, symbol token_sym);

    //functions

    bool is_option_in_ballot(name option_name, vector<option> options);
    bool is_option_in_receipt(name option_name, vector<name> options_voted);
    int get_option_index(name option_name, vector<option> options);
    bool has_token_balance(name voter, symbol sym);
    void update_votes(name voter);
    void unvote_option();
    asset get_staked_tlos(name owner);
    bool applied_rebalance(name ballot_name, asset delta, vector<name> options_to_rebalance);
    
    void upsert_balance();
    void apply_delta();
    
};
