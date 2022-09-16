// Models in this file are based on and ported from the madmom project:
// @inproceedings{madmom,
//    Title = {{madmom: a new Python Audio and Music Signal Processing Library}},
//    Author = {B{\"o}ck, Sebastian and Korzeniowski, Filip and Schl{\"u}ter, Jan and Krebs, Florian and Widmer, Gerhard},
//    Booktitle = {Proceedings of the 24th ACM International Conference on
//    Multimedia},
//    Month = {10},
//    Year = {2016},
//    Pages = {1174--1178},
//    Address = {Amsterdam, The Netherlands},
//    Doi = {10.1145/2964284.2973795}
// }
//
// Models ported to Rust by Eric Van Albert
//
// The content of this file is distributed under the following license:
// 
// Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// 
// The short version of this license:
// 
// You are free to:
// 
//   Share: copy and redistribute the material in any medium or format
//   Adapt: remix, transform, and build upon the material
// 
// Under the following terms:
// 
//   Attribution:   You must give appropriate credit, provide a link to the
//                  license, and indicate if changes were made.
//   NonCommercial: You must not use the material for commercial purposes.
//   ShareAlike:    If you remix, transform, or build upon the material, you must
//                  distribute your contributions under the same license as the
//                  original.
// 
// All legal details can be found here:
// http://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
// 
// If you want to include any of these files (or a variation or modification
// thereof) or technology which utilises them in a commercial product, please
// contact both Gerhard Widmer at http://www.cp.jku.at/people/widmer/.
// and Eric Van Albert at https://eric.van.al/.

use nalgebra::{DVector};
use crate::beat_tracking::layers::*;

pub struct NeuralNetwork {
    layer1: LSTMLayer, // INPUT_SIZE = N_FILTERS * 2
    layer2: LSTMLayer,
    layer3: LSTMLayer,
    layer4: FeedForwardLayer, // OUTPUT_SIZE = 1
}

impl NeuralNetwork {
    pub fn new(
        layer1: LSTMLayer,
        layer2: LSTMLayer,
        layer3: LSTMLayer,
        layer4: FeedForwardLayer,
    ) -> Self {
        Self {
            layer1,
            layer2,
            layer3,
            layer4,
        }
    }

    pub fn reset(&mut self) {
        self.layer1.reset();
        self.layer2.reset();
        self.layer3.reset();
        // layer4 is a FeedForwardLayer
        // and therefore doesn't have any state to reset()
    }

    pub fn process(
        &mut self,
        data: &DVector<f32> // size N_FILTERS * 2
    ) -> f32 {
        let layer1_result = self.layer1.process(data);
        let layer2_result = self.layer2.process(&layer1_result);
        let layer3_result = self.layer3.process(&layer2_result);
        let layer4_result = self.layer4.process(&layer3_result);
        assert_eq!(layer4_result.len(), 1); // NN should produce a scalar output
        layer4_result[0]
    }
}

