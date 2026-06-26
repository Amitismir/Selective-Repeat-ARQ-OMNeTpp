//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
simple SrcNode
{
    parameters:
        int windowSize;
        double timeout @unit(s);
        @signal[channelUtilization](type="double");
        @statistic[channelUtilization](title="Channel Utilization Efficiency"; source="channelUtilization"; record=vector,last);
    gates:
        input in;
        output out;
}

simple DstNode
{
    parameters:
        int windowSize;
    gates:
        input in;
        output out;
}

network SRNetwork
{
    @display("bgb=471,246");
    types:
        channel TranscontinentalLink extends ned.DatarateChannel
        {
            datarate = 100kbps;
            delay = 50ms;
            per = default(0.0);
        }
    submodules:
        src: SrcNode {
            @display("p=50,156");
        }
        dst: DstNode {
            @display("p=391,62");
        }
    connections:
        src.out --> TranscontinentalLink --> dst.in;
        dst.out --> TranscontinentalLink --> src.in;
}
