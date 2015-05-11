/*****************************************************************************

            Copyright (C), 1998-2014, Huawei Tech. Co., Ltd.

 *****************************************************************************
  file			:  models/helpers/link-helper.cc
  version		: 5G Simulator 0.1
  author		: Petar Djukic
  create date	: 2012-08-28
  description	: The class of functor to calculate path loss.
  history		:
-----------------------------------------------------------------------------
 1. Petar Djukic     2012-08-28       Create for The class of functor to calculate path loss.
-----------------------------------------------------------------------------
*****************************************************************************/

#include <fstream>
#include "simcore/create.h"
#include "models/medium/wireless-link.h"
#include "models/helpers/wireless-link-helper.h"
#include "models/radio/resource-grid.h"
#include "models/phyconst/phy-constants.h"
#include "math/decibel.h"
#include "math/stats/histogram.h"
#include "math/stats/utils.h"
#include "io/parameters.h"
#include "io/stats-collector.h"
#include "io/link-logger.h"
#include "io/stats-manager.h"
#include "math/random/rng-manager.h"
#include "math/random/unifrndi.h"
#include "math/decibel.h"

using namespace std;

WirelessLinkHelper::WirelessLinkHelper(const Parameters& p,
                                       const Ptr<ChannelFactory>& channel_factory,
                                       const Ptr<IPathLossMap>& path_loss_map) :
    channel_factory_(channel_factory),
    path_loss_map_(path_loss_map),
    max_transmit_power_(),
    noise_power_per_tone_(PhyConst().NoisePowerPerTone_W),
    nonsimulated_interference_per_time_(),
    path_loss_scaling_db_(p.path_loss_scaling_db),
    simulated_portion_of_interference_(p.ota.simulated_portion_of_interference_for_link_pruning),
    ho_margin_db_(p.deployment.ho_margin_db),
    ho_margin_rng_(RNGMgr().GetSeed(RS_HO_MARGIN))
{
    ASSERT(simulated_portion_of_interference_ >= 0.0);
    ASSERT(simulated_portion_of_interference_ <= 1.0);
    if (p.deployment.scenario == HETNET)
    {
        macro_forward_channel_type_ = p.deployment.macro.channel_type;
        macro_reverse_channel_type_ = p.deployment.macro.channel_type;
        macro_penetration_loss_db_ = p.deployment.macro.penetration_loss_db;
        micro_forward_channel_type_ = p.deployment.micro.channel_type;
        micro_reverse_channel_type_ = p.deployment.micro.channel_type;
        micro_penetration_loss_db_ = p.deployment.micro.penetration_loss_db;
    }
    else if (p.deployment.scenario == MACRO_ONLY)
    {
        macro_forward_channel_type_ = p.deployment.macro.channel_type;
        macro_reverse_channel_type_ = p.deployment.macro.channel_type;
        macro_penetration_loss_db_ = p.deployment.macro.penetration_loss_db;

    }
    else if (p.deployment.scenario == MICRO_ONLY)
    {
        micro_forward_channel_type_ = p.deployment.micro.channel_type;
        micro_reverse_channel_type_ = p.deployment.micro.channel_type;
        micro_penetration_loss_db_ = p.deployment.micro.penetration_loss_db;
    }
}

//TRAIL [Deployment:005] Assign channel to each link and calculate gain of each link including antenna gain, shadowing and path loss
Ptr<WirelessLink> WirelessLinkHelper::Create(const Ptr<AntennaSet>& src_antenna_set,
        const Ptr<AntennaSet>& dst_antenna_set,
        const Ptr<FastFadingChannel>& channel_prototype)
{
    if (src_antenna_set->antenna_type == MACRO)
    {
        max_transmit_power_ = Par().bbu_mac.maximum_macrobbu_power_W;
    }
    else if (src_antenna_set->antenna_type == MICRO)
    {
        max_transmit_power_ = Par().bbu_mac.maximum_microbbu_power_W;
    }
    else
    {
    	std::cout<<"Antenna type is incorrect!"<<std::endl;
    	ASSERT(0);
    }
    ASSERT(max_transmit_power_ > 0.0);
    Ptr<WirelessLink> link = ::Create<WirelessLink>( Par(),
                             src_antenna_set, dst_antenna_set, channel_prototype, path_loss_map_);

    link->LoadChannel();
    link->UnloadChannel();

    link->UpdateLocations(); //Gain is calculated here.


    return link;
}

//TRAIL [Deployment:004] Create links and assign fast fading channel to each link
void WirelessLinkHelper::CreateLinks(const AntennaSetContainer& src_radios,
                                     const AntennaSetContainer& dst_radios)
{
    Ptr<FastFadingChannel> macro_channel_prototype;
    Ptr<FastFadingChannel> micro_channel_prototype;
    if(Par().deployment.macro.number_of_rrus>0)
    {
    	macro_channel_prototype = channel_factory_->Create(macro_forward_channel_type_);
    }
    if(Par().deployment.micro.number_of_rrus>0)
    {
    	micro_channel_prototype = channel_factory_->Create(micro_forward_channel_type_);
    }
    for (AntennaSetContainer::const_iterator dst_it = dst_radios.begin(); dst_it != dst_radios.end(); ++dst_it)
    {
        LinksSortedByGain links_sorted_by_gain;
        double total_gain = 0.0;
        Ptr<AntennaSet> dst_antenna_set = (*dst_it).second;
        for (AntennaSetContainer::const_iterator src_it = src_radios.begin(); src_it != src_radios.end(); ++src_it)
        {
            Ptr<AntennaSet> src_antenna_set = (*src_it).second;
            Ptr<WirelessLink> wireless_link;
            if(src_antenna_set->antenna_type==MACRO)
            {
            	wireless_link = Create(src_antenna_set, dst_antenna_set, macro_channel_prototype);
            }
            else if(src_antenna_set->antenna_type==MICRO)
            {
            	wireless_link = Create(src_antenna_set, dst_antenna_set, micro_channel_prototype);
            }
            else
            {
            	std::cout<<"Antenna type is incorrect!"<<std::endl;
            	ASSERT(0);
            }
            links_sorted_by_gain.insert(wireless_link);
            total_gain += wireless_link->GetGain();
        }

        //TODO: Prune links here
        LinksSortedByGain::iterator first_simulated_link_it = links_sorted_by_gain.begin();
        LinksSortedByGain::iterator last_simulated_link_it  = links_sorted_by_gain.end();

        double signal_gain  = (*first_simulated_link_it)->GetGain();
        double total_interference_gain = total_gain - signal_gain;

        // ! dont do this here. In case this is not the serving cell. wireless_links_.push_back(*(first_simulated_link_it));

        // next find out which links do not need to be simulated.
        ++first_simulated_link_it;
        double non_simulated_interferer_gains = 0.0;

        if ( simulated_portion_of_interference_ < 1.0 )
        {
            LinksSortedByGain::iterator first_nonsimulated_link_it =
                FindFirstNonSimulatedLink(first_simulated_link_it, last_simulated_link_it,
                                          total_interference_gain );

            last_simulated_link_it = first_nonsimulated_link_it;

            for ( auto link = first_nonsimulated_link_it; link != links_sorted_by_gain.end();
                  ++link )
            {
                non_simulated_interferer_gains += (*link)->GetGain();
            }
        }
        double max_per_tone_power      = max_transmit_power_ / (ResourceGrid::GetResourceGrid().useful_bandwidth / ResourceGrid::GetResourceGrid().tone_bandwidth);
        dst_antenna_set->per_tone_non_simulated_power = non_simulated_interferer_gains * max_per_tone_power;


        first_simulated_link_it = links_sorted_by_gain.begin(); // put this iterator back at the highest gain link

        // add a check for HO margin and if necessary, swap the top (serving)cell with another one.
        if (ho_margin_db_ > 0.0)
        {
            double ho_margin_factor_linear = DecibelsToLinear(-ho_margin_db_);


            // will check top four cells in the list sorted by gain
            int highest_list_pos = 0;
            int positions_checked = 0;
            double GainOfFirstLink_Times_HO_Margin =  ((*first_simulated_link_it)->GetGain()) * ho_margin_factor_linear;

            while (positions_checked < 4)
            {
                ++first_simulated_link_it;
                ++positions_checked;
                if (((*first_simulated_link_it)->GetGain()) > GainOfFirstLink_Times_HO_Margin)
                {
                    ++highest_list_pos;  // link is within HO margin
                }
                else
                {
                    break; // this link is not within HO margin, no point in continuing the checking (sorted by gain)
                }
            }

            if ( highest_list_pos > 0 )
            {
                // draw a random number between 0 and highest list pos to choose serving cell
                UnifRndi HO_random_cell_selection(0, highest_list_pos, &ho_margin_rng_ );
                int servingListPosition = HO_random_cell_selection();
                if ( servingListPosition > 0 )
                {
                    ASSERT(servingListPosition < 4); // we only checked the first four cells.
                    LinksSortedByGain::iterator selected_simulated_link_it = links_sorted_by_gain.begin();
                    for (int idxit  = 0; idxit < servingListPosition; idxit++)
                    {
                        ++selected_simulated_link_it;
                    }
                    wireless_links_.push_back(*(selected_simulated_link_it));
                    links_sorted_by_gain.erase(selected_simulated_link_it);
                }
            }
        }  // end of if covering HO margin.

        first_simulated_link_it = links_sorted_by_gain.begin(); // put this iterator back at the highest gain link
        std::copy(first_simulated_link_it, last_simulated_link_it, std::back_inserter(wireless_links_));



    } // end of for over dst_it



}
LinksSortedByGain::iterator WirelessLinkHelper::FindFirstNonSimulatedLink(
    LinksSortedByGain::iterator first_simulated_link_it,
    LinksSortedByGain::iterator last_simulated_link_it,
    double total_interference) const
{
    double sum_of_simulated_interference_ = 0.0;
    double interference_goal = total_interference * simulated_portion_of_interference_;
    while ( first_simulated_link_it != last_simulated_link_it )
    {
        double link_gain = (*first_simulated_link_it)->GetGain();
        if (sum_of_simulated_interference_ < interference_goal)
        {
            sum_of_simulated_interference_ += link_gain;
            ++first_simulated_link_it;
        }
        else
        { break; }
    }
    return first_simulated_link_it;
}

void WirelessLinkHelper::CreateDstAdjacencyList()
{
    for (WirelessLinkContainer::const_iterator it = wireless_links_.begin(); it != wireless_links_.end(); ++it)
    {
        const Ptr<WirelessLink>& link = (*it);
        dst_adjacency_map[link->DstID()].insert(link);
    }

    if (Par().save_adjacency_list)
    {
        std::string srcadjlistfilename = Par().output_directory + "sumimo-links-dst.txt";
        SaveDstAdjacencyList(srcadjlistfilename);
    }
}

void WirelessLinkHelper::SaveDstAdjacencyList(const std::string& file_name) const
{
    const int kCW = StatsCollector::kCW;  // column width
    std::ofstream fout(file_name);

    fout << "% All-to-all links\n"
         << "% RRU-id  = RRU antenna id\n"
         << "% UE-id   = UE id\n"
         << "% RRU-x   = RRU antenna location, x-coordinate [m]\n"
         << "% RRU-y   = RRU antenna location, y-coordinate [m]\n"
         << "% UE-x    = UE location, x-coordinate [m]\n"
         << "% UE-y    = UE location, y-coordinate [m]\n"
         << "% dist    = RRU-UE distance [m]\n"
         << "% ch-gain = channel gain [dB]\n"
         << "% antGain = antenna gain [dB]\n"
         << "% gain    = total link gain [dB]\n\n";
    fout << '%' << setw(kCW - 1) << "RRU-id" << ' ' << setw(kCW) << "UE-id"
         << ' ' << setw(kCW) << "RRU-x"    << ' ' << setw(kCW) << "RRU-y"
         << ' ' << setw(kCW) << "UE-x"     << ' ' << setw(kCW) << "UE-y"
         << ' ' << setw(kCW) << "dist"     << ' ' << setw(kCW) << "ch-gain"
         << ' ' << setw(kCW) << "antGain"
         << ' ' << setw(kCW) << "gain"     << '\n' << std::setprecision(5);

    for (AdjacencyMap::const_iterator node_it = dst_adjacency_map.begin();
         node_it != dst_adjacency_map.end(); ++node_it)
    {
        for (LinksSortedByGain::const_iterator link_it = (*node_it).second.begin();
             link_it != (*node_it).second.end(); ++link_it)
        {
            const Ptr<WirelessLink>& link = (*link_it);
            Point src_location = link->src_antenna_set_->location.NearestLocationFor(link->dst_antenna_set_->location);
            Point dst_location = link->dst_antenna_set_->location.NearestLocationFor(link->src_antenna_set_->location);

            fout << setw(kCW) << link->SrcID()
                 << ' ' << setw(kCW) << link->DstID()
                 << ' ' << setw(kCW) << roundTo<2>(src_location.x)
                 << ' ' << setw(kCW) << roundTo<2>(src_location.y)
                 << ' ' << setw(kCW) << dst_location.x
                 << ' ' << setw(kCW) << dst_location.y
                 << ' ' << setw(kCW) << link->GetDistance()
                 << ' ' << setw(kCW) << link->GetLogDomainChannelGain()
                 << ' ' << setw(kCW) << link->GetLogDomainAntennaGain()
                 << ' ' << setw(kCW) << LinearToDecibels(link->GetGain()) << '\n';
        }
    }
    fout.close();
}


void WirelessLinkHelper::CalculateGeometry()
{
    double transmit_BW = ResourceGrid::GetResourceGrid().useful_bandwidth;
    for (AdjacencyMap::iterator node_it = dst_adjacency_map.begin(); node_it != dst_adjacency_map.end(); ++node_it)
    {
        LinksSortedByGain::iterator linkit = (*node_it).second.begin();
        double denominator = noise_power_per_tone_ * DecibelsToLinear(path_loss_scaling_db_) + (*linkit)->GetNonsimulatedInterfAtDestination();
        for (LinksSortedByGain::iterator link_it = (*node_it).second.begin(); link_it != (*node_it).second.end(); ++link_it)
        {
            double gain     = (*link_it)->GetGain();
            double tx_power = max_transmit_power_ / (transmit_BW / ResourceGrid::GetResourceGrid().tone_bandwidth); // per tone based Tx power
            double rx_power = gain * tx_power;
            denominator += rx_power;
        }
        for (LinksSortedByGain::iterator link_it = (*node_it).second.begin(); link_it != (*node_it).second.end(); ++link_it)
        {
            double tx_power  = max_transmit_power_ / (transmit_BW / ResourceGrid::GetResourceGrid().tone_bandwidth);
            double rx_power  = (*link_it)->GetGain() * tx_power;
            double snr       = rx_power / (denominator - rx_power);
            Ptr<WirelessLink>  link = (*link_it);
            link->snr_geometry_db = LinearToDecibels(snr);
//            link->loss_geometry_db = (*link_it)->GetlossGain(); // for calibration
        }
    }
}

DVector WirelessLinkHelper::GetGeometry()
{
    DVector geometry_snr(dst_adjacency_map.size());
    int node_number = 0;
    for (AdjacencyMap::iterator node_it = dst_adjacency_map.begin(); node_it != dst_adjacency_map.end(); ++node_it)
    {
        Ptr<WirelessLink>  link = (*(*node_it).second.begin());
        geometry_snr(node_number++) = link->snr_geometry_db;
    }
    return geometry_snr;
}

DVector WirelessLinkHelper::GetlossGeometry()
{
    DVector geometry_loss(dst_adjacency_map.size());
//    int node_number = 0;
//    for (AdjacencyMap::iterator node_it = dst_adjacency_map.begin(); node_it != dst_adjacency_map.end(); ++node_it)
//    {
//        Ptr<WirelessLink>  link = (*(*node_it).second.begin());
//        geometry_loss(node_number++) = link->loss_geometry_db;
//    }
    return geometry_loss;
}

void WirelessLinkHelper::WriteLocationsAndGeometryFiles()
{
    const int kCW = StatsCollector::kCW;  // column width
    const string& path_for_results = Par().output_directory;
    Histogram* geo_hist = 0;
    ofstream* msloc = 0;
    ofstream* usergeomfile = 0;
//    ofstream* userlossfile = 0; // for calibration

//    Histogram* loss_hist = 0; // for calibration
//    Histogram* zod_hist = 0; // for calibration
//    Histogram* zsd_hist = 0; // for calibration
//    Histogram* zsa_hist = 0; // for calibration

    if ( Par().results.ue_locations_enabled )
    {
        msloc = new ofstream( path_for_results + "/msloc.txt" );
        *msloc << "% UE initial locations and associations\n"
               << '%' << setw(kCW - 1) << "UE-id" << ' ' << setw(kCW) << "RRU-id"
               << ' ' << setw(kCW)   << "UE-x"  << ' ' << setw(kCW) << "UE-y" << '\n'
               << setprecision(6);
    }

    if ( Par().results.geometry.enabled )
    {
        geo_hist = new Histogram( Par().results.geometry.hist, "Geometry per UE", "dB" );
//        loss_hist = new Histogram(-150, -50, 1, "LOSS per UE ", "dB" ); // for calibration
//        zod_hist = new Histogram(80, 140, 0.3, "Elevation angle of Departure ", "degree" ); // for calibration
//        zsd_hist = new Histogram(0, 60, 1, "RMS zenith spread of departure angles (ZSD) ", "degree" ); // for calibration
//        zsa_hist = new Histogram(0, 60, 1, "RMS zenith spread of arrival angles (ZSA) ", "degree" ); // for calibration
    }
    if ( Par().results.geometry.outputallusergeom )
    {
        usergeomfile = new ofstream( path_for_results + "/allusergeom.txt" );
        *usergeomfile << "% All UE geometry and associations\n"
                      << '%'
                      << setw(kCW) << "UE-id"
                      << setw(kCW + 8) << "Geom"
                      << setw(kCW + 3) << "RRU-id"
                      << '\n'
                      << setprecision(6);
    }

//    // for calibration
//    if ( Par().results.geometry.outputallusergeom )
//    {
//        userlossfile = new ofstream( path_for_results + "/alluserloss.txt" );
//        *userlossfile << "% All UE coupling loss and associations\n"
//                      << '%'
//                      << setw(kCW) << "UE-id"
//                      << setw(kCW + 8) << "Loss"
//                      << setw(kCW + 3) << "RRU-id"
//                      << '\n'
//                      << setprecision(6);
//    }


    for (AdjacencyMap::iterator node_it = dst_adjacency_map.begin();
         node_it != dst_adjacency_map.end(); ++node_it)
    {
        const Ptr<WirelessLink>& link = *(node_it->second.begin());
        const Ptr<FastFadingChannel> chan = (link->GetChannel());

        // for calibration
//        if (zod_hist && Par().CellInStats(link->src_antenna_set_->antenna_id) )
//       {
//          LineOfSight chk_los = path_loss_map_->GetLOS(link->src_antenna_set_->antenna_id, link->dst_antenna_set_->antenna_id);
//          if ( chk_los == LOS)
//          {
//              DMatrix ang;
//              ang =  chan->outputZODInfo();
//              int size_i = ang.rows();
//              int size_j = ang.cols();
//              for ( int i = 0 ;  i < size_i ; ++i)
//              {
//                 for ( int j = 0; j < size_j; ++j)
//                 {
//                    zod_hist->Add( ang(i, j) );
//                 }
//              }
//                zod_hist->Add( chan->outputZODInfo() );
//          }
//       }

//        if ( zsd_hist && Par().CellInStats(link->src_antenna_set_->antenna_id) ) // for calibration
//        {
//            chan->setLOSstatus(path_loss_map_->GetLOS(link->src_antenna_set_->antenna_id, link->dst_antenna_set_->antenna_id));
//            chan->setLocationStatus(link->dst_antenna_set_->antenna_lt);
//            zsd_hist->Add( chan->outputZSDInfo());
//        }

//        if ( zsa_hist && Par().CellInStats(link->src_antenna_set_->antenna_id) ) // for calibration
//        {
//            chan->setLOSstatus(path_loss_map_->GetLOS(link->src_antenna_set_->antenna_id, link->dst_antenna_set_->antenna_id));
//            chan->setLocationStatus(link->dst_antenna_set_->antenna_lt);
//            zsa_hist->Add( chan->outputZSAInfo());
//        }

        if ( geo_hist && Par().CellInStats(link->src_antenna_set_->antenna_id) )
        {
            geo_hist->Add( link->snr_geometry_db );
        }

//        if ( loss_hist && Par().CellInStats(link->src_antenna_set_->antenna_id) ) // for calibration
//        {
//            loss_hist->Add( link->loss_geometry_db );
//        }

        if ( msloc )
        {
            *msloc << setw(kCW) << link->DstID()
                   << ' ' << setw(kCW) << link->SrcID()
                   << ' ' << setw(kCW) << link->dst_antenna_set_->location.ActualLocation().x
                   << ' ' << setw(kCW) << link->dst_antenna_set_->location.ActualLocation().y
                   << '\n';
        }
        if ( usergeomfile )
        {
            *usergeomfile << setw(kCW) << link->DstID()
                          << setw(kCW + 8) << link->snr_geometry_db
                          << setw(kCW + 3) << link->SrcID()
                          << '\n';
        }

//        // for calibration
//        if ( userlossfile )
//        {
//            *userlossfile << setw(kCW) << link->DstID()
//                          << setw(kCW + 8) << link->loss_geometry_db
//                          << setw(kCW + 3) << link->SrcID()
//                          << '\n';
//        }
    }

    if ( geo_hist )
    {
        ofstream geomfout( path_for_results + "geometry.txt" );
        geo_hist->Dump( geomfout );
        delete geo_hist;
    }

//    if ( loss_hist ) // for calibration
//    {
//        ofstream lossmfout( path_for_results + "loss_cdf.txt" );
//        loss_hist->Dump( lossmfout );
//        delete loss_hist;
//    }
//    if ( zod_hist ) // for calibration
//    {
//        ofstream zodmfout( path_for_results + "zod_cdf.txt" );
//        zod_hist->Dump( zodmfout );
//        delete zod_hist;
//    }
//
//    if ( zsd_hist ) // for calibration
//   {
//       ofstream zsdcdf( path_for_results + "zsd_cdf.txt" );
//       zsd_hist->Dump( zsdcdf );
//       delete zsd_hist;
//   }
//
//    if ( zsa_hist ) // for calibration
//   {
//       ofstream zsdcdf( path_for_results + "zsa_cdf.txt" );
//       zsa_hist->Dump( zsdcdf );
//       delete zsa_hist;
//   }

    delete msloc;
    delete usergeomfile;
//    delete userlossfile;
}
